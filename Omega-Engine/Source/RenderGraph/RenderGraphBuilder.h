#pragma once

#include "RenderGraph/RenderGraphPass.h"
#include "RenderGraph/Resources.h"
#include "utility/CString.h"

namespace VulkanAPI
{
class Image;
class ImageView;
}

namespace OmegaEngine
{

class RenderGraph;
class RenderGraphPass;

/**
 * @brief Useful helper functions for building the rendergraph
 */
class RenderGraphBuilder
{
public:
    RenderGraphBuilder(RenderGraph* rGraph, RenderGraphPass* rPass);

    /**
     * @ creates a texture resource for using as a render target in a graphics  pass
     */
    ResourceHandle createRenderTarget(
        Util::String name,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const vk::ImageUsageFlagBits usageBits = vk::ImageUsageFlagBits::eSampled,
        uint32_t levels = 1,
        uint32_t layers = 1);

    ResourceHandle importRenderTarget(
        Util::String name,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const uint8_t samples,
        VulkanAPI::ImageView& view);
    
    /**
     @brief Sets the specified renderpass flag
     */
    void setRenderPassFlag(const RenderPassFlags& flag);
    
    /**
     @brief Creates a buffer resource for using as a render target in a compute pass
     */
    ResourceHandle createBuffer(BufferResource* buffer);

    /**
     * @brief Adds a input attachment to the render pass. There must be a corresponding output
     * attachment otherwise returns UINT64_MAX as error.
     */
    AttachmentHandle addReader(Util::String name);

    /**
     * @brief Adds a output attachment such as a colour/depth/stencil attachment to the pass
     */
    AttachmentHandle addWriter(Util::String name, const ResourceHandle resource);

    /**
     * @brief Adds a function to execute each frame for this renderpass
     * @param func The function to execute. Must be of the format (void*)(RenderPassContext&)
     *
     */
    void addExecute(ExecuteFunc&& func);

    /**
     * @brief Sets the clear colour for all attachments for this pass
     */
    void setClearColour(const OEMaths::colour4& clearCol);

    /**
     * @brief Sets the depth clear for this pass
     */
    void setDepthClear(float depthClear);

private:
    // a reference to the graph and pass we are building
    RenderGraph* rGraph = nullptr;
    RenderGraphPass* rPass = nullptr;
};

} // namespace OmegaEngine
