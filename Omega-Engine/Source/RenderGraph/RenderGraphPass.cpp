#include "RenderGraphPass.h"

#include "RenderGraph/RenderGraph.h"
#include "VulkanAPI/Utility.h"
#include "utility/Logger.h"

namespace OmegaEngine
{


RenderGraphPass::RenderGraphPass(
    Util::String name, const Type type, RenderGraph& rGraph, const uint32_t index)
    : rGraph(rGraph), name(name), type(type), index(index)
{
}

ResourceHandle RenderGraphPass::addRead(const ResourceHandle input)
{
    // make sure that this handle doesn't already exsist in the list
    // This is just a waste of memory having reduntant resources
    auto iter = std::find(reads.begin(), reads.end(), input);
    if (iter != reads.end())
    {
        return *iter;
    }
    reads.emplace_back(input);
    return input;
}

ResourceHandle RenderGraphPass::addWrite(const ResourceHandle output)
{
    // make sure that this handle doesn't already exsist in the list
    // This is just a waste of memory having reduntant resources
    auto iter = std::find(writes.begin(), writes.end(), output);
    if (iter != writes.end())
    {
        return *iter;
    }
    writes.emplace_back(output);
    return output;
}

void RenderGraphPass::prepare(VulkanAPI::VkDriver& driver, RenderGraphPass* parent)
{
    switch (type)
    {
        case Type::Graphics: {
            // used for signyfing to the subpass the reference ids associated with it
            std::vector<uint32_t> inputRefs, outputRefs;
            uint32_t depthRef = UINT32_MAX;

            // Note: This is integrated yet - merged passes needs some work - will be user defined
            // rather than automagic as I want to reduce the potential for tons of boilerplate and
            // bugs if this isn't a merged pass, create a new renderpass. Otherwise, use the parent
            // pass
            if (!flags.testBit(VulkanAPI::SubpassFlags::Merged) ||
                flags.testBit(VulkanAPI::SubpassFlags::MergedBegin))
            {
                context.rpass = rGraph.createRenderPass();
                // the pass will also need a framebuffer - the framebuffer could be handled by the
                // pass?
                context.framebuffer = rGraph.createFrameBuffer();
            }

            VulkanAPI::RenderPass* rpass;
            if (parent)
            {
                rpass = rGraph.getRenderpass(parent->context.rpass);
            }
            else
            {
                rpass = rGraph.getRenderpass(context.rpass);
            }

            auto& resources = rGraph.getResources();

            // add the output attachments
            for (ResourceHandle handle : writes)
            {
                ResourceBase* base = resources[handle];
                if (base->type == ResourceBase::ResourceType::Texture)
                {
                    TextureResource* tex = static_cast<TextureResource*>(resources[handle]);

                    // bake the texture
                    if (tex->width != maxWidth || tex->height != maxHeight)
                    {
                        // not exactly a reason to fatal error maybe, but warn the user
                        // maybe do a blit here instead? The answer is yes, TODO!!!!!
                        tex->width = maxWidth;
                        tex->height = maxHeight;
                        LOGGER_WARN(
                            "There appears to be some discrepancy between this passes resource "
                            "dimensions\n");
                    }
                    tex->bake(driver);

                    if (!VulkanAPI::VkUtil::isDepth(tex->format) ||
                        !VulkanAPI::VkUtil::isStencil(tex->format))
                    {
                        outputRefs.emplace_back(tex->referenceId);
                    }
                    else
                    {
                        // only one depth/stencil ref per pass
                        assert(depthRef == UINT32_MAX);
                        depthRef = tex->referenceId;
                    }

                    // add a attachment
                    rpass->addOutputAttachment(
                        tex->format, tex->referenceId, tex->clearFlags, tex->samples);
                }
                else if (base->type == ResourceBase::ResourceType::Imported)
                {
                }
            }

            // Add a subpass. If this is a merged pass, then this will be added to the parent
            rpass->addSubPass(inputRefs, outputRefs);
            if (parent)
            {
                rpass->addSubpassDependency(parent->flags);
            }
            else
            {
                rpass->addSubpassDependency(flags);
            }
            break;
        }
        case Type::Compute: {
            break;
        }
    }
}

void RenderGraphPass::bake()
{
    // create the renderpass
    rGraph.getRenderpass(context.rpass)->prepare();
}

void RenderGraphPass::addExecute(ExecuteFunc&& func)
{
    execFunc = std::move(func);
}

void RenderGraphPass::setClearColour(const OEMaths::colour4& colour)
{
    context.clearCol = colour;
}

void RenderGraphPass::setDepthClear(const float depth)
{
    context.depthClear = depth;
}

} // namespace OmegaEngine