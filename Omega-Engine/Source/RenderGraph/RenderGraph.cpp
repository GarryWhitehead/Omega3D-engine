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

RenderGraph::RenderGraph(VulkanAPI::VkDriver& driver) : driver(driver)
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
    Util::String name,
    const VulkanAPI::Image& image,
    VulkanAPI::ImageView& imageView,
    const uint32_t width,
    const uint32_t height)
{
   // TODO
    return ResourceHandle {0};
}

void RenderGraph::CullResourcesAndPasses(ResourceBase* resource)
{
    // the render pass that outputs to this resource
    // RenderGraphPass* rpass = resource->outputPass;

    /* if (rpass)
     {
         --rpass->refCount;

         // this pass has no more write attahment dependencies so can be culled
         if (rpass->refCount == 0)
         {
             for (ResourceHandle& handle : rpass->inputs)
             {
                 ResourceBase* rsrc = resources[handle];
                 --resource->inputCount;
                 if (rsrc->inputCount == 0)
                 {
                     // no input dependencies, so see if we can cull this pass
                     CullResourcesAndPasses(rsrc);
                 }
             }
         }
     }*/
}

AttachmentHandle RenderGraph::addAttachment(AttachmentInfo& info)
{
    attachments.emplace_back(info);
    return attachments.size() - 1;
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

    std::vector<uint32_t> reorderedPasses;
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
        for (ResourceHandle handle : curPass->writes)
        {
            resourceStack.emplace_back(handle);
        }

        while (!resourceStack.empty())
        {
            ResourceHandle curHandle = resourceStack.back();
            ResourceBase* res = resources[curHandle];

            passStack.emplace_back(res->writer);
        }
    }

    std::reverse(reorderedPasses.begin(), reorderedPasses.end());

    // now tidy up the passes by removing duplicates - the first time the pass is seen will be
    // its position within# the ordered passes, all subsequent passes will be removed
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

        // TODO: this needs some work
        rpass.flags |= VulkanAPI::SubpassFlags::TopOfPipeline;

        // passes with no refences are treated as culled
        uint32_t refId = 0;
        uint32_t maxWidth = std::numeric_limits<uint32_t>::min();
        uint32_t maxHeight = std::numeric_limits<uint32_t>::min();

        for (const ResourceHandle handle : rpass.writes)
        {
            ResourceBase* base = resources[handle];
            if (base->type == ResourceBase::ResourceType::Texture)
            {
                TextureResource* tex = reinterpret_cast<TextureResource*>(base);

                // used by the attachment descriptor
                tex->referenceId = refId++;

                // use the resource with max dimensions
                maxWidth = std::max(maxWidth, tex->width);
                maxHeight = std::max(maxHeight, tex->height);

                // if the texture format is depth/stencil, then set as depth/stencil read,
                // otherwise, colour read/write
                if (tex->isDepthFormat() || tex->isStencilFormat())
                {
                    if (tex->isDepthFormat())
                    {
                        rpass.flags |= VulkanAPI::SubpassFlags::DepthRead;
                        tex->imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
                    }
                    if (tex->isStencilFormat())
                    {
                        rpass.flags |= VulkanAPI::SubpassFlags::StencilRead;
                        tex->imageUsage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
                    }
                }
                else
                {
                    // assume must be a colour format
                    rpass.flags |= VulkanAPI::SubpassFlags::ColourRead;
                    tex->imageUsage |= vk::ImageUsageFlagBits::eColorAttachment;
                }
            }
        }

        rpass.maxWidth = maxWidth;
        rpass.maxHeight = maxHeight;

        // TODO: need to deal with merging passes too!
    }

    return true;
}

void RenderGraph::initRenderPass()
{
    for (RenderGraphPass& rpass : rGraphPasses)
    {
        switch (rpass.type)
        {
            case RenderGraphPass::Type::Graphics: {
                std::vector<VulkanAPI::ImageView*> views(rpass.writes.size());
                for (size_t i = 0; i < rpass.writes.size(); ++i)
                {
                    AttachmentHandle handle = rpass.writes[i];
                    assert(handle < attachments.size());
                    AttachmentInfo& attach = attachments[handle];

                    // bake the resources
                    VulkanAPI::ImageView* view =
                        reinterpret_cast<VulkanAPI::ImageView*>(attach.bake(driver, *this));
                    views[i] = view;
                }

                // check if this is merged - will have child passes associated with it
                if (rpass.flags.testBit(VulkanAPI::SubpassFlags::Merged))
                {
                    if (rpass.flags.testBit(VulkanAPI::SubpassFlags::MergedBegin))
                    {
                        // prepare this pass first
                        rpass.prepare(driver);
                    }
                    else
                    {
                        RenderGraphPass* rootPass = &rGraphPasses[rpass.mergedRootIdx];
                        rpass.prepare(driver, rootPass);
                    }

                    // if the ifnal pass is the merged list, we can create it
                    if (rpass.flags.testBit(VulkanAPI::SubpassFlags::MergedEnd))
                    {
                        rpass.bake();
                    }
                }
                else
                {
                    // create the renderpass
                    rpass.prepare(driver);
                    rpass.bake();
                }

                // create the framebuffer - this is linked to the renderpass
                VulkanAPI::FrameBuffer* fbuffer = getFramebuffer(rpass.context.framebuffer);
                VulkanAPI::RenderPass* renderpass = getRenderpass(rpass.context.rpass);
                fbuffer->prepare(*renderpass, views, rpass.maxWidth, rpass.maxHeight, 1);

                VulkanAPI::CmdPool* cmdPool = driver.getCbManager().getMainPool();
                rpass.context.cmdBuffer = cmdPool->createPrimaryCmdBuffer();
                break;
            }

            case RenderGraphPass::Type::Compute: {
                // TODO
                break;
            }
        }
    }
}

bool RenderGraph::prepare()
{
    if (!rebuild)
    {
        return true;
    }

    // start by optimising the graph and filling out the structure
    if (!compile())
    {
        return false;
    }

    // init the renderpass resources - command buffer, frame buffers, etc.
    initRenderPass();

    rebuild = false;
    return true;
}

void RenderGraph::execute()
{
    // iterate over all passes and execute the registered callback function
    for (RenderGraphPass& rpass : rGraphPasses)
    {
        // start the render pass
        VulkanAPI::CmdBufferManager& manager = driver.getCbManager();

        assert(rpass.context.cmdBuffer);
        VulkanAPI::FrameBuffer* fbuffer = getFramebuffer(rpass.context.framebuffer);
        VulkanAPI::RenderPass* renderpass = getRenderpass(rpass.context.rpass);
        rpass.context.cmdBuffer->begin();
        manager.beginRenderpass(rpass.context.cmdBuffer, *renderpass, *fbuffer);

        rpass.execFunc(rpass.context);
    }
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

RPassHandle RenderGraph::createRenderPass()
{
    renderpasses.emplace_back(std::make_unique<VulkanAPI::RenderPass>(driver.getContext()));
    return RPassHandle {renderpasses.size() - 1};
}

FBufferHandle RenderGraph::createFrameBuffer()
{
    framebuffers.emplace_back(std::make_unique<VulkanAPI::FrameBuffer>(driver.getContext()));
    return FBufferHandle {framebuffers.size() - 1};
}

VulkanAPI::RenderPass* RenderGraph::getRenderpass(const RPassHandle& handle)
{
    assert(handle.get() < renderpasses.size());
    return renderpasses[handle.get()].get();
}

VulkanAPI::FrameBuffer* RenderGraph::getFramebuffer(const FBufferHandle& handle)
{
    assert(handle.get() < framebuffers.size());
    return framebuffers[handle.get()].get();
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
