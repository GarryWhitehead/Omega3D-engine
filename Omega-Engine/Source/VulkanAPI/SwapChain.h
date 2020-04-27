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

#include "Types/NativeWindowWrapper.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Platform/Surface.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/CommandBuffer.h"

#include <vector>

namespace OmegaEngine
{
class OEWindowInstance;
}

namespace VulkanAPI
{

// forward declerations
class VkDriver;

struct SwapchainContext
{
    std::unique_ptr<Image> image;
    std::unique_ptr<ImageView> view;
    std::unique_ptr<CmdBuffer> cmdBuffer;
    vk::Fence fence;
};

class Swapchain
{

public:
    Swapchain();
    ~Swapchain();

    // moveable
    Swapchain(Swapchain&&) = default;
    Swapchain& operator=(Swapchain&&) = default;

    /**
     * @brief Creates the swapchain using the supplied native window surface
     * Note: The surface is obtained by calling **createSurface**.
     * Note: This function must be called before using with a **Scene** object.
     * @param context A prepared vulkan device object
     * @param surface A surface object for the given platform
     */
    bool prepare(VkContext& context, Platform::SurfaceWrapper& surface);

    void destroy(VkContext& context);
    
    // static functions
    /**
     * @brief Creates a KHR surface object using a native window pointer.
     * Note: The native window is obtained from a source such as glfw (used in the application for
     * now)
     * @param window A wrapper containg all the info needed to create a surface and swapchain
     * @param instance A vulkan instance wrapper obtained from calling **createInstance**
     */
    static Platform::SurfaceWrapper
    createSurface(OmegaEngine::OEWindowInstance* window, vk::Instance& instance);

    vk::SwapchainKHR& get();
    uint32_t getExtentsHeight() const;
    uint32_t getExtentsWidth() const;
    ImageView& getImageView(const uint8_t index);
    vk::Format& getFormat();
    
    friend class VkDriver;

private:
    /// creates the image views for the swapchain
    void prepareImageViews(VkContext& context, const vk::SurfaceFormatKHR& surfaceFormat);

private:
    // the dimensions of the current swapchain
    vk::Extent2D extent;

    // a swapchain based on the present surface type
    vk::SwapchainKHR swapchain;
    
    vk::SurfaceFormatKHR surfaceFormat;
    std::vector<SwapchainContext> contexts;
    std::unique_ptr<CmdBuffer> scCmdBuffer;
};
} // namespace VulkanAPI
