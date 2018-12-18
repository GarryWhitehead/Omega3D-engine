#include "VkSwapChain.h"
#include "VulkanCore/VulkanDevice.h"
#include "VulkanCore/vulkan_utility.h"
#include "utility/file_log.h"
#include <algorithm>

namespace VkInitilisation
{
	std::vector<VkImage> PrepareSwapchain(VkDevice device,
		VkPhysicalDevice physDevice,
		VkInstance instance,
		VkSurfaceKHR surface,
		int graphIndex,
		int presentIndex,
		VkSurfaceFormatKHR& format,
		VkPresentModeKHR& mode,
		VkExtent2D& extent,
		VkSwapchainKHR& swapChain,
		uint32_t screenWidth,
		uint32_t screenHeight)
	{
		// Get the basic surface properties of the physical device
		uint32_t surfaceCount;
		VkSurfaceCapabilitiesKHR capabilities;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDevice, surface, &capabilities);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceCount, nullptr);

		std::vector<VkSurfaceFormatKHR> formats(surfaceCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physDevice, surface, &surfaceCount, formats.data());

		// And then get the presentation modes available for this device
		uint32_t presentCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentCount, nullptr);

		std::vector<VkPresentModeKHR> modes(presentCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physDevice, surface, &presentCount, modes.data());

		// make sure that we have suitable swap chain extensions available before continuing
		if (formats.empty() || modes.empty()) {
			g_filelog->WriteLog("Critcal error! Unable to locate suitable swap chains on device.");
			exit(EXIT_FAILURE);
		}

		// Next step is to determine the surface format. Ideally undefined format is preffered so we can set our own, otherwise
		// we will go with one that suits our colour needs - i.e. 8bitBGRA and SRGB.
		if ((formats.size() > 0) && (formats[0].format == VK_FORMAT_UNDEFINED))
		{
			format.format = VK_FORMAT_B8G8R8A8_UNORM;
			format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		}
		else
		{
			for (auto& format : formats)
				if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				{
					format = format;
					break;
				}
		}

		// And then the presentation format - the preferred format is triple buffering
		for (auto& mode : modes)
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)													// our preferred triple buffering mode
			{
				mode = mode;
				break;
			}
			else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			{
				mode = mode;																	// immediate mode is only supported on some drivers.
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
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
			imageCount = capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format.format;
		createInfo.imageColorSpace = format.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = mode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		// if the graphics and presentation aren't the same then use concurrent sharing mode
		uint32_t queueIndicies[] = { graphIndex, presentIndex };

		if (graphIndex != presentIndex)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueIndicies;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		// And finally, create the swap chain
		VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain));

		// Get the image loactions created when creating the swap chains
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);

		std::vector<VkImage> images(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data());

		return images;
	}

	std::vector<VkImageView> PrepareImageViews(VkDevice device, std::vector<VkImage>& images, VkSurfaceFormatKHR format)
	{
		std::vector<VkImageView> imageViews;

		for (int c = 0; c < images.size(); ++c)
		{
			VkImageView imageView = VulkanUtility::InitImageView(images[c], format.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, device);
			imageViews.push_back(imageView);
		}

		return imageViews;
	}

	void PrepareSemaphores(VkDevice device, VkSemaphore image, VkSemaphore present)
	{
		image = VulkanUtility::CreateSemaphore(device);
		present = VulkanUtility::CreateSemaphore(device);
	}


	void DestroySwapChain(VkDevice device, VkSwapchainKHR swapChain, std::vector<VkImageView> imageViews)
	{
		for (int c = 0; c < imageViews.size(); ++c)
			vkDestroyImageView(device, imageViews[c], nullptr);

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}
}