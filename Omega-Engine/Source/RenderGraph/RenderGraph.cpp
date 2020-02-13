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
    RenderGraphPass rPass {name, type, *this};
    rGraphPasses.emplace_back(rPass);
    RenderGraphBuilder builder {this, &rGraphPasses.back()};
    return builder;
}

ResourceHandle RenderGraph::addResource(ResourceBase* resource)
{
    resources.emplace_back(resource);
    return resources.size() - 1;
}

void RenderGraph::CullResourcesAndPasses(ResourceBase* resource)
{
    // the render pass that outputs to this resource
    RenderGraphPass* rpass = resource->outputPass;

    if (rpass)
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
    }
}

AttachmentHandle RenderGraph::addAttachment(AttachmentInfo& info)
{
    attachments.emplace_back(info);
    return attachments.size() - 1;
}

bool RenderGraph::compile()
{

    for (RenderGraphPass& rpass : rGraphPasses)
    {
        if (rpass.type == RenderGraphPass::Type::Graphics)
        {
            // the number of resources this pass references too
            rpass.refCount = rpass.outputs.size();

            // work out how many resources read from this resource
            for (ResourceHandle& handle : rpass.inputs)
            {
                assert(handle < resources.size());
                ResourceBase* resource = resources[handle];
                ++resource->inputCount;
            }

            // for the outputs, set the pass which writes to this resource
            for (ResourceHandle& handle : rpass.outputs)
            {
                if (handle >= resources.size())
                {
                    LOGGER_ERROR("There are more output attachments than there are resources.");
                    return false;
                }
                ResourceBase* resource = resources[handle];
                resource->outputPass = &rpass;
            }
        }
    }

    // tidy up reference counts - total references, both inputs and outputs
    for (ResourceBase* resource : resources)
    {
        resource->refCount += resource->inputCount;
    }

    // finialise the attachments
    size_t passCount = rGraphPasses.size();

    for (size_t i = 0; i < passCount; ++i)
    {
        RenderGraphPass& rpass = rGraphPasses[i];
        
        // TODO: this needs some work
        rpass.flags |= VulkanAPI::SubpassFlags::TopOfPipeline;

        // passes with no refences are treated as culled
        uint32_t refId = 0;
        uint32_t maxWidth = std::numeric_limits<uint32_t>::min();
        uint32_t maxHeight = std::numeric_limits<uint32_t>::min();

        for (const ResourceHandle handle : rpass.outputs)
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

        // if the pass has an input (reader), then we treat this as a merged subpass.
        if (!rpass.inputs.empty())
        {
            // a few sanity checks - you can't read from the first pass
            if (i == 0)
            {
                LOGGER_ERROR(
                    "You are trying to read from the first pass - you must have an output!");
                return false;
            }
            // there must be enough outputs from the last pass to read from
            RenderGraphPass& prevPass = rGraphPasses[i - 1];
            if (rpass.inputs.size() > prevPass.outputs.size())
            {
                LOGGER_ERROR("There are more inputs than there are outputs!");
                return false;
            }

            // inform the renderpass that this is a merged subpass
            rpass.flags |= VulkanAPI::SubpassFlags::Merged;

            // Work out the root pass of this merge - this pass will contain the list of subpasses
            if (!prevPass.flags.testBit(VulkanAPI::SubpassFlags::Merged))
            {
                rpass.mergedRootIdx = i - 1;
                prevPass.flags.testBit(VulkanAPI::SubpassFlags::MergedBegin);
            }
            else
            {
                assert(prevPass.mergedRootIdx != UINT64_MAX);
                rpass.mergedRootIdx = prevPass.mergedRootIdx;

                // check if this is the final pass in the merge
                if (i < passCount - 1)
                {
                    RenderGraphPass& nextPass = rGraphPasses[i + 1];
                    if (!nextPass.inputs.empty())
                    {
                        rpass.flags |= VulkanAPI::SubpassFlags::MergedEnd;
                    }
                }
            }
        }
    }
    return true;
}

void RenderGraph::initRenderPass()
{
    for (RenderGraphPass& rpass : rGraphPasses)
    {
        if (rpass.refCount == 0)
        {
            continue;
        }

        switch (rpass.type)
        {
            case RenderGraphPass::Type::Graphics: {
                std::vector<VulkanAPI::ImageView*> views(rpass.outputs.size());
                for (size_t i = 0; i < rpass.outputs.size(); ++i)
                {
                    AttachmentHandle handle = rpass.outputs[i];
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
        manager.beginRenderpass(
            rpass.context.cmdBuffer, *renderpass, *fbuffer);

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
