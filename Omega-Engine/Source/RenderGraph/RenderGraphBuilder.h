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
        const vk::ImageUsageFlags usageBits = vk::ImageUsageFlagBits::eSampled,
        uint32_t mipLevels = 1,
        uint32_t faceCount = 1);

    ResourceHandle importRenderTarget(
        Util::String name,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const uint8_t samples,
        VulkanAPI::ImageView& view);
    
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
