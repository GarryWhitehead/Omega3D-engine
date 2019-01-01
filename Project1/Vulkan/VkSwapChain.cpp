#include "VkSwapChain.h"

#include "Utility/logger.h"
#include <algorithm>

namespace VulkanAPI
{
	SwapchainKHR::SwapchainKHR(vk::Device device,
		vk::PhysicalDevice gpu,
		vk::Instance instance,
		int graphIndex,
		int presentIndex,
		uint32_t screenWidth,
		uint32_t screenHeight)
	{
		// Get the basic surface properties of the physical device
		uint32_t surfaceCount;
		

		VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));

		vk::SurfaceCapabilitiesKHR capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
		std::vector<vk::SurfaceFormatKHR> formats = gpu.getSurfaceFormatsKHR(surface);
		std::vector<vk::PresentModeKHR> present_modes = gpu.getSurfacePresentModesKHR(surface);

		// make sure that we have suitable swap chain extensions available before continuing
		if (formats.empty() || present_modes.empty()) {
			LOGGER_ERROR("Critcal error! Unable to locate suitable swap chains on device.");
			throw std::runtime_error("Unable to initialise KHR swapchain");
		}

		// Next step is to determine the surface format. Ideally undefined format is preffered so we can set our own, otherwise
		// we will go with one that suits our colour needs - i.e. 8bitBGRA and SRGB.
		vk::SurfaceFormatKHR req_surf_format;

		if ((formats.size() > 0) && (formats[0].format == vk::Format::eUndefined))
		{
			req_surf_format.format = vk::Format::eB8G8R8A8Unorm;
			req_surf_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
		}
		else
		{
			for (auto& format : formats)
				if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
				{
					req_surf_format = format;
					break;
				}
		}

		// And then the presentation format - the preferred format is triple buffering
		vk::PresentModeKHR req_mode;
		for (auto& mode : present_modes)
			if (mode == vk::PresentModeKHR::eMailbox)													// our preferred triple buffering mode
			{
				req_mode = mode;
				break;
			}
			else if (mode == vk::PresentModeKHR::eImmediate)
			{
				req_mode = mode;																	// immediate mode is only supported on some drivers.
				break;
			}

		// Finally set the resoultion of the swap chain buffers
		// First of check if we can manually set the dimension - some GPUs allow this by setting the max as the size of uint32
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			extent = capabilities.currentExtent;									// go with the automatic settings
		else
		{
			extent = { screenWidth, screenHeight };
			extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, extent.width));
			extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, extent.height));
		}

		// Get the number of possible images we can send to the queue
		uint32_t imageCount = capabilities.minImageCount + 1;								 // adding one as we would like to implement triple buffering
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
			imageCount = capabilities.maxImageCount;
		}

		// if the graphics and presentation aren't the same then use concurrent sharing mode
		std::vector<uint32_t> queue_family_indicies;
		vk::SharingMode sharing_mode = vk::SharingMode::eExclusive;

		if (graphIndex != presentIndex)
		{
			sharing_mode = vk::SharingMode::eConcurrent;
			queue_family_indicies.push_back(graphIndex);
			queue_family_indicies.push_back(presentIndex);
		}

		vk::SwapchainCreateInfoKHR createInfo({},
			surface,
			imageCount,
			req_surf_format.format,
			req_surf_format.colorSpace,
			extent,
			1, vk::ImageUsageFlagBits::eColorAttachment,
			sharing_mode,
			queue_family_indicies.empty() ? 0 : static_cast<uint32_t>(queue_family_indicies.size()),
			queue_family_indicies.empty() ? nullptr : queue_family_indicies.data(),
			capabilities.currentTransform,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			req_mode, VK_TRUE, VK_NULL_HANDLE);

		// And finally, create the swap chain
		VK_CHECK_RESULT(device.createSwapchainKHR(&createInfo, nullptr, &swapchain));

		// Get the image loactions created when creating the swap chains
		std::vector<vk::Image> images = device.getSwapchainImagesKHR(swapchain);

		for (int c = 0; c < images.size(); ++c)
		{
			vk::ImageView imageView = VulkanUtility::InitImageView(images[c], req_surf_format.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, device);
			image_views.push_back(imageView);
		}
	}

	SwapchainKHR::~SwapchainKHR()
	{
		for (int c = 0; c < image_views.size(); ++c)
			vkDestroyImageView(device, imageViews[c], nullptr);

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}
}