#include "SwapChain.h"

#include "Utility/logger.h"

#include "VulkanAPI/VkDriver.h"

#include "Types/NativeWindowWrapper.h"

#include <algorithm>

namespace VulkanAPI
{
Swapchain::Swapchain()
{
}

Platform::SurfaceWrapper Swapchain::createSurface(OmegaEngine::NativeWindowWrapper& window, Instance& instance)
{
	Platform::SurfaceWrapper wrapper(window, instance);
	return wrapper;
}

bool Swapchain::prepare(VkDriver& context, Platform::SurfaceWrapper& surface)
{
	vk::Device device = context.getDevice();
	vk::PhysicalDevice gpu = context.getPhysicalDevice();

	// Get the basic surface properties of the physical device
	uint32_t surfaceCount = 0;

	vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
	std::vector<vk::SurfaceFormatKHR> surfaceFormats = gpu.getSurfaceFormatsKHR(surface);
	std::vector<vk::PresentModeKHR> presentModes = gpu.getSurfacePresentModesKHR(surface);

	// make sure that we have suitable swap chain extensions available before continuing
	if (surfaceFormats.empty() || presentModes.empty())
	{
		LOGGER_ERROR("Critcal error! Unable to locate suitable swap chains on device.");
		return false;
	}

	// Next step is to determine the surface format. Ideally undefined format is preffered so we can set our own, otherwise
	// we will go with one that suits our colour needs - i.e. 8bitBGRA and SRGB.
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
			if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
			{
				requiredSurfaceFormats = format;
				break;
			}
		}
	}
	vk::SurfaceFormatKHR surfaceFormat = requiredSurfaceFormats;

	// And then the presentation format - the preferred format is triple buffering
	vk::PresentModeKHR requiredPresentMode;
	for (auto& mode : presentModes)
	{
		if (mode == vk::PresentModeKHR::eMailbox)    // our preferred triple buffering mode
		{
			requiredPresentMode = mode;
			break;
		}
		else if (mode == vk::PresentModeKHR::eImmediate)
		{
			requiredPresentMode = mode;    // immediate mode is only supported on some drivers.
			break;
		}
	}

	// Finally set the resoultion of the swap chain buffers
	// First of check if we can manually set the dimension - some GPUs allow this by setting the max as the size of uint32
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		this->extent = capabilities.currentExtent;    // go with the automatic settings
	}
	else
	{
		this->extent.width =
		    std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, surface.getWidth()));
		this->extent.height =
		    std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, surface.getHeight()));
	}

	// Get the number of possible images we can send to the queue
	uint32_t imageCount =
	    capabilities.minImageCount + 1;    // adding one as we would like to implement triple buffering
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	// if the graphics and presentation aren't the same then use concurrent sharing mode
	std::vector<uint32_t> queueFamilyIndicies;
	vk::SharingMode sharingMode = vk::SharingMode::eExclusive;

	uint32_t graphFamilyIdx = context.getQueueIndex(VkDriver::QueueType::Graphics);
	uint32_t presentFamilyIdx = context.getQueueIndex(VkDriver::QueueType::Present);

	if (graphFamilyIdx != presentFamilyIdx)
	{
		sharingMode = vk::SharingMode::eConcurrent;
		queueFamilyIndicies.push_back(graphFamilyIdx);
		queueFamilyIndicies.push_back(presentFamilyIdx);
	}

	vk::SwapchainCreateInfoKHR createInfo(
	    {}, surface.get(), imageCount, requiredSurfaceFormats.format, requiredSurfaceFormats.colorSpace, extent, 1,
	    vk::ImageUsageFlagBits::eColorAttachment, sharingMode, 0, nullptr, capabilities.currentTransform,
	    vk::CompositeAlphaFlagBitsKHR::eOpaque, requiredPresentMode, VK_TRUE, {});

	// And finally, create the swap chain
	VK_CHECK_RESULT(device.createSwapchainKHR(&createInfo, nullptr, &swapchain));

	// Get the image loactions created when creating the swap chains
	std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain);

	for (int c = 0; c < images.size(); ++c)
	{
		ImageView imageView;
		imageView.create(device, images[c], requiredSurfaceFormats.format, vk::ImageAspectFlagBits::eColor,
		                 vk::ImageViewType::e2D);
		imageViews.emplace_back(std::move(imageView));
	}

	prepareSwapchainPass();

	return true;
}

Swapchain::~Swapchain()
{
	for (int c = 0; c < imageViews.size(); ++c)
	{
		device.destroyImageView(imageViews[c].getImageView(), nullptr);
	}

	device.destroySwapchainKHR(swapchain, nullptr);
}



void Swapchain::prepareSwapchainPass()
{
	// depth image
	vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);

	depthTexture = std::make_unique<Texture>(device, gpu);
	depthTexture->createEmptyImage(depthFormat, extent.width, extent.height, 1,
	                               vk::ImageUsageFlagBits::eDepthStencilAttachment);

	renderpass = std::make_unique<RenderPass>(device);
	renderpass->addAttachment(surfaceFormat.format, VulkanAPI::FinalLayoutType::PresentKHR, true);
	renderpass->addAttachment(depthFormat, VulkanAPI::FinalLayoutType::Auto);
	renderpass->prepareRenderPass();

	// create presentation renderpass/framebuffer for each swap chain image
	for (uint32_t i = 0; i < imageViews.size(); ++i)
	{

		// create a frame buffer for each swapchain image
		std::vector<vk::ImageView> views{ imageViews[i].getImageView(), depthTexture->getImageView() };
		renderpass->prepareFramebuffer(static_cast<uint32_t>(views.size()), views.data(), extent.width, extent.height);
	}
}

}    // namespace VulkanAPI
