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

void RenderGraphPass::setFlag(const RenderPassFlags& flag)
{
    renderPassFlags |= flag;
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

void RenderGraphPass::prepare(VulkanAPI::VkDriver& driver)
{
    switch (type)
    {
        case Type::Graphics: {
            // used for signyfing to the subpass the reference ids associated with it
            std::vector<uint32_t> inputRefs, outputRefs;
            uint32_t depthRef = UINT32_MAX;
            
            context.rpass = rGraph.createRenderPass();
            VulkanAPI::RenderPass* rpass = rGraph.getRenderpass(context.rpass);
            auto& resources = rGraph.getResources();

            // add the output attachments
            std::vector<VulkanAPI::ImageView*> views(writes.size());
            for (size_t i = 0; i < writes.size(); ++i)
            {
                ResourceHandle& handle = writes[i];
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
                    views[i] = reinterpret_cast<VulkanAPI::ImageView*>(tex->bake(driver));

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
            rpass->addSubpassDependency(flags);
            
            // create the framebuffer - this is linked to the renderpass
            context.framebuffer = rGraph.createFrameBuffer();
            VulkanAPI::FrameBuffer* fbuffer = rGraph.getFramebuffer(context.framebuffer);
            fbuffer->prepare(*rpass, views, maxWidth, maxHeight, 1);
            
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

void RenderGraphPass::resetSkipExecFlag()
{
    skipPassExec = false;
}

} // namespace OmegaEngine
