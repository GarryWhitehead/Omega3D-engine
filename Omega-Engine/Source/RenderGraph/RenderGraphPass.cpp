#include "RenderGraphPass.h"

#include "RenderGraph/RenderGraph.h"
#include "VulkanAPI/Utility.h"
#include "utility/Logger.h"

namespace OmegaEngine
{


RenderGraphPass::RenderGraphPass(Util::String name, const Type type, RenderGraph& rGraph)
    : rGraph(rGraph), name(name), type(type)
{
}

ResourceHandle RenderGraphPass::addInput(const ResourceHandle input)
{
    // make sure that this handle doesn't already exsist in the list
    // This is just a waste of memory having reduntant resources
    auto iter = std::find(inputs.begin(), inputs.end(), input);
    if (iter != inputs.end())
    {
        return *iter;
    }
    inputs.emplace_back(input);
    return input;
}

ResourceHandle RenderGraphPass::addOutput(const ResourceHandle output)
{
    // make sure that this handle doesn't already exsist in the list
    // This is just a waste of memory having reduntant resources
    auto iter = std::find(outputs.begin(), outputs.end(), output);
    if (iter != outputs.end())
    {
        return *iter;
    }
    outputs.emplace_back(output);
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

            // if this isn't a merged pass, create a new renderpass. Otherwise, use the parent pass
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
            for (ResourceHandle handle : outputs)
            {
                ResourceBase* base = resources[handle];
                assert(base->type == ResourceBase::ResourceType::Texture);
                TextureResource* tex = static_cast<TextureResource*>(resources[handle]);

                // bake the texture
                if (tex->width != maxWidth || tex->height != maxHeight)
                {
                    // not exactly a reason to fatal error maybe, but warn the user
                    // maybe do a blit here instead? The answer is yes, TODO!!!!!
                    tex->width = maxWidth;
                    tex->height = maxHeight;
                    LOGGER_WARN("There appears to be some discrepancy between this passes resource "
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

            // input attachments
            for (ResourceHandle handle : inputs)
            {
                ResourceBase* base = resources[handle];
                assert(base->type == ResourceBase::ResourceType::Texture);
                TextureResource* tex = static_cast<TextureResource*>(resources[handle]);

                // for clear flags, we always 'store' for the loadOp
                // check the format here, if stencil set the stencil Op flags??
                tex->clearFlags.attachLoad = VulkanAPI::RenderPass::LoadType::Store;

                inputRefs.emplace_back(tex->referenceId);
                rpass->addInputRef(tex->referenceId);
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