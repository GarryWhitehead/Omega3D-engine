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

#include "SwapChain.h"

#include "Types/NativeWindowWrapper.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

#include <algorithm>

namespace VulkanAPI
{
Swapchain::Swapchain()
{
}

Swapchain::~Swapchain()
{
}

void Swapchain::destroy(VkContext& context)
{
    context.device.destroy(swapchain);
}

Platform::SurfaceWrapper
Swapchain::createSurface(OmegaEngine::OEWindowInstance* window, vk::Instance& instance)
{
    Platform::SurfaceWrapper wrapper {*window, instance};
    return wrapper;
}

bool Swapchain::prepare(VkContext& context, Platform::SurfaceWrapper& surface)
{
    vk::Device device = context.device;
    vk::PhysicalDevice gpu = context.physical;

    // Get the basic surface properties of the physical device
    vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface.get());
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = gpu.getSurfaceFormatsKHR(surface.get());
    std::vector<vk::PresentModeKHR> presentModes = gpu.getSurfacePresentModesKHR(surface.get());

    // make sure that we have suitable swap chain extensions available before continuing
    if (surfaceFormats.empty() || presentModes.empty())
    {
        LOGGER_ERROR("Critcal error! Unable to locate suitable swap chains on device.");
        return false;
    }

    // Next step is to determine the surface format. Ideally undefined format is preffered so we can
    // set our own, otherwise we will go with one that suits our colour needs - i.e. 8bitBGRA and
    // SRGB.
    vk::SurfaceFormatKHR requiredSurfaceFormats;

    if ((surfaceFormats.size() > 0) && (surfaceFormats[0].format == vk::Format::eUndefined))
    {
        requiredSurfaceFormats.format = vk::Format::eB8G8R8A8Unorm;
        requiredSurfaceFormats.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    }
    else
    {
        for (auto& format : surfaceFormats)
        {
            if (format.format == vk::Format::eB8G8R8A8Unorm &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                requiredSurfaceFormats = format;
                break;
            }
        }
    }
    surfaceFormat = requiredSurfaceFormats;

    // And then the presentation format - the preferred format is triple buffering
    vk::PresentModeKHR requiredPresentMode;
    for (auto& mode : presentModes)
    {
        if (mode == vk::PresentModeKHR::eMailbox) // our preferred triple buffering mode
        {
            requiredPresentMode = mode;
            break;
        }
        else if (mode == vk::PresentModeKHR::eImmediate)
        {
            requiredPresentMode = mode; // immediate mode is only supported on some drivers.
            break;
        }
    }

    // Finally set the resoultion of the swap chain buffers
    // First of check if we can manually set the dimension - some GPUs allow this by setting the max
    // as the size of uint32
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        this->extent = capabilities.currentExtent; // go with the automatic settings
    }
    else
    {
        this->extent.width = std::max(
            capabilities.minImageExtent.width,
            std::min(capabilities.maxImageExtent.width, surface.getWidth()));
        this->extent.height = std::max(
            capabilities.minImageExtent.height,
            std::min(capabilities.maxImageExtent.height, surface.getHeight()));
    }

    // Get the number of possible images we can send to the queue
    uint32_t imageCount =
        capabilities.minImageCount + 1; // adding one as we would like to implement triple buffering
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    // if the graphics and presentation aren't the same then use concurrent sharing mode
    std::vector<uint32_t> queueFamilyIndicies;
    vk::SharingMode sharingMode = vk::SharingMode::eExclusive;

    uint32_t graphFamilyIdx = context.queueFamilyIndex.graphics;
    uint32_t presentFamilyIdx = context.queueFamilyIndex.present;

    if (graphFamilyIdx != presentFamilyIdx)
    {
        sharingMode = vk::SharingMode::eConcurrent;
        queueFamilyIndicies.push_back(graphFamilyIdx);
        queueFamilyIndicies.push_back(presentFamilyIdx);
    }

    vk::SwapchainCreateInfoKHR createInfo(
        {},
        surface.get(),
        imageCount,
        requiredSurfaceFormats.format,
        requiredSurfaceFormats.colorSpace,
        extent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        sharingMode,
        0,
        nullptr,
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        requiredPresentMode,
        VK_TRUE,
        {});

    // And finally, create the swap chain
    VK_CHECK_RESULT(device.createSwapchainKHR(&createInfo, nullptr, &swapchain));

    prepareImageViews(context, surfaceFormat);

    return true;
}

void Swapchain::prepareImageViews(VkContext& context, const vk::SurfaceFormatKHR& surfaceFormat)
{
    vk::Device device = context.device;

    // Get the image loactions created when creating the swap chain
    std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain);

    for (size_t i = 0; i < images.size(); ++i)
    {
        SwapchainContext scContext;
        scContext.view = std::make_unique<ImageView>(context);
        scContext.image = std::make_unique<Image>(
            context, images[i], surfaceFormat.format, extent.width, extent.height);
        scContext.view->create(device, *scContext.image);
        contexts.emplace_back(std::move(scContext));
    }
}

ImageView& Swapchain::getImageView(const uint8_t index)
{
    assert(index < contexts.size());
    return *contexts[index].view;
}

vk::SwapchainKHR& Swapchain::get()
{
    return swapchain;
}

uint32_t Swapchain::getExtentsHeight() const
{
    return extent.height;
}

uint32_t Swapchain::getExtentsWidth() const
{
    return extent.width;
}

vk::Format& Swapchain::getFormat()
{
    return surfaceFormat.format;
}

} // namespace VulkanAPI
