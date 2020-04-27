/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "RenderGraph.h"

#include "RenderGraph/RenderGraphBuilder.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

#include <algorithm>
#include <assert.h>
#include <limits>
#include <stdint.h>

namespace OmegaEngine
{

RenderGraph::RenderGraph(VulkanAPI::VkDriver* driver, OERenderer* renderer) : context(driver, renderer, this)
{
}

RenderGraph::~RenderGraph()
{
}

RenderGraphBuilder RenderGraph::createPass(Util::String name, const RenderGraphPass::Type type)
{
    // add the pass to the list
    RenderGraphPass rPass {name, type, *this, static_cast<uint32_t>(rGraphPasses.size())};
    rGraphPasses.emplace_back(rPass);
    RenderGraphBuilder builder {this, &rGraphPasses.back()};
    return builder;
}

ResourceHandle RenderGraph::addResource(ResourceBase* resource)
{
    resources.emplace_back(resource);
    return resources.size() - 1;
}

ResourceHandle RenderGraph::moveResource(const ResourceHandle from, const ResourceHandle to)
{
    ResourceBase* res = resources[from];
    aliases.push_back({from, to});
    return addResource(res);
}

ResourceHandle RenderGraph::importResource(
    const Util::String& name, VulkanAPI::ImageView& imageView, const uint32_t width, const uint32_t height, const vk::Format format, const uint8_t samples)
{
    ImportedResource* ires = new ImportedResource{name, width, height, format, samples, imageView};
    resources.emplace_back(ires);
    return resources.size() - 1;
}

AttachmentHandle RenderGraph::addAttachment(AttachmentInfo& info)
{
    attachments.emplace_back(info);
    return attachments.size() - 1;
}

void RenderGraph::reset()
{
    rGraphPasses.clear();
    reorderedPasses.clear();
    resources.clear();
    aliases.clear();
    attachments.clear();
}

bool RenderGraph::compile()
{
    // TODO: deal with aliases here (moved resources)

    for (RenderGraphPass& rgPass : rGraphPasses)
    {
        // set the number of reads for each resource
        for (const ResourceHandle& handle : rgPass.reads)
        {
            ResourceBase* res = resources[handle];
            res->readCount++;
        }

        // set which pass is writing to the resource
        for (const ResourceHandle& handle : rgPass.writes)
        {
            ResourceBase* res = resources[handle];
            res->writer = &rgPass;
        }
    }

    reorderedPasses.clear();
    std::vector<RenderGraphPass*> passStack;

    // start by re-ordering the passes - starting from the "root" node (i.e. the backbuffer), work
    // backwards and find all passes which write to this node. Then take these passes and find the
    // resource writes, and so on.... traverse bottom up - the last pass will (must) be the
    // backbuffer

    uint64_t lastIdx = rGraphPasses.size() - 1;
    RenderGraphPass& lastPass = rGraphPasses[lastIdx];
    reorderedPasses.emplace_back(lastIdx);

    // we make a assertion here that the last pass can only write to the backbuffer
    assert(lastPass.writes.size() == 1);
    ResourceHandle curHandle = lastPass.writes[0];
    ResourceBase* bbRes = resources[curHandle];

    passStack.emplace_back(bbRes->writer);

    while (!passStack.empty())
    {
        RenderGraphPass* curPass = passStack.back();
        assert(curPass);
        passStack.pop_back();

        reorderedPasses.emplace_back(curPass->index);

        std::vector<ResourceHandle> resourceStack;
        for (ResourceHandle handle : curPass->reads)
        {
            resourceStack.emplace_back(handle);
        }

        while (!resourceStack.empty())
        {
            ResourceHandle curHandle = resourceStack.back();
            resourceStack.pop_back();
            ResourceBase* res = resources[curHandle];
            assert(res);

            passStack.emplace_back(res->writer);
        }
    }

    std::reverse(reorderedPasses.begin(), reorderedPasses.end());

    // now tidy up the passes by removing duplicates - the first time the pass is seen will be
    // its position within the ordered passes, all subsequent passes will be removed
    std::unordered_set<uint32_t> seen;
    auto newEnd = std::remove_if(
        reorderedPasses.begin(), reorderedPasses.end(), [&seen](const uint32_t& value) {
            if (seen.find(value) != std::end(seen))
            {
                return true;
            }
            seen.insert(value);
            return false;
        });
    reorderedPasses.erase(newEnd, reorderedPasses.end());

    // Now we have the optimal pass order, finialise the attachments
    size_t passCount = reorderedPasses.size();

    for (size_t i = 0; i < passCount; ++i)
    {
        RenderGraphPass& rpass = rGraphPasses[reorderedPasses[i]];

        // passes with no refences are treated as culled
        uint32_t maxWidth = std::numeric_limits<uint32_t>::min();
        uint32_t maxHeight = std::numeric_limits<uint32_t>::min();

        for (const ResourceHandle handle : rpass.writes)
        {
            ResourceBase* base = resources[handle];
            if (base->type == ResourceBase::ResourceType::Texture)
            {
                TextureResource* tex = reinterpret_cast<TextureResource*>(base);

                // use the resource with max dimensions
                maxWidth = std::max(maxWidth, tex->width);
                maxHeight = std::max(maxHeight, tex->height);

                // if the texture format is depth/stencil, then set as depth/stencil read,
                // otherwise, colour read/write
                if (tex->isDepthFormat() || tex->isStencilFormat())
                {
                    if (tex->isDepthFormat())
                    {
                        tex->imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
                    }
                    if (tex->isStencilFormat())
                    {
                        tex->imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
                    }
                }
                else
                {
                    // assume must be a colour format
                    tex->imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
                }
            }
        }

        rpass.maxWidth = maxWidth;
        rpass.maxHeight = maxHeight;

        // TODO: need to deal with merging passes too!
    }
    
    // sort out the vulkan renderpasses and render targets ready for execution
    initRenderPass();
    
    return true;
}

void RenderGraph::initRenderPass()
{
    for (const uint32_t& rpassIdx : reorderedPasses)
    {
        RenderGraphPass& rpass = rGraphPasses[rpassIdx];
        
        switch (rpass.type)
        {
            case RenderGraphPass::Type::Graphics: {
                
                // create the renderpass
                rpass.prepare(*context.driver);
                break;
            }

            case RenderGraphPass::Type::Compute: {
                // TODO
                break;
            }
        }
    }
}

void RenderGraph::execute()
{
    VulkanAPI::CBufferManager& manager = context.driver->getCbManager();
    VulkanAPI::CmdBuffer* cmdBuffer = manager.getCmdBuffer();

    cmdBuffer->begin();
    
    // iterate over all passes and execute the registered callback function. The backbuffer draw pass is ensured to be the last pass after reordering, this will be executed outside the loop as uses a seperate cmd buffer
    for(size_t i = 0; i  < reorderedPasses.size() - 1; ++i)
    {
        const uint32_t rpassIdx = reorderedPasses[i];
        RenderGraphPass& rpass = rGraphPasses[rpassIdx];
        
        rpass.execFunc(rpass.context, context);
    }

    manager.flushCmdBuffer();
    
    // now execute the backbuffer draw pass
    const uint32_t rpassIdx = reorderedPasses.back();
    RenderGraphPass& rpass = rGraphPasses[rpassIdx];
    rpass.execFunc(rpass.context, context);
}

AttachmentHandle RenderGraph::findAttachment(const Util::String& req)
{
    uint64_t index = 0;
    for (AttachmentInfo& attach : attachments)
    {
        if (attach.name.compare(req))
        {
            return index;
        }
        ++index;
    }
    return UINT64_MAX;
}

std::vector<ResourceBase*>& RenderGraph::getResources()
{
    return resources;
}

ResourceBase* RenderGraph::getResource(const ResourceHandle handle)
{
    return resources[handle];
}

} // namespace OmegaEngine
