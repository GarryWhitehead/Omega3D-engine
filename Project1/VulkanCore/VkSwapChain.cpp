#include "VkSwapChain.h"
#include "VulkanCore/VulkanDevice.h"
#include "VulkanCore/vulkan_utility.h"
#include "utility/file_log.h"
#include <algorithm>

VkSwapChain::VkSwapChain(VkSurfaceKHR surf, VkDevice device, GLFWwindow *win) : 
	surface(surf),
	device(device),
	window(win)
{
	assert(device != VK_NULL_HANDLE);
}


VkSwapChain::~VkSwapChain()
{
	Destroy();
}

void VkSwapChain::PrepareSwapChain(VulkanDevice *p_vkDevice, VkInstance instance, uint32_t screenWidth, uint32_t screenHeight)
{
	// Get the basic surface properties of the physical device
	uint32_t surfaceCount;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_vkDevice->physDevice, surface, &capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_vkDevice->physDevice, surface, &surfaceCount, nullptr);

	formats.resize(surfaceCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(p_vkDevice->physDevice, surface, &surfaceCount, formats.data());

	// And then get the presentation modes available for this device
	uint32_t presentCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_vkDevice->physDevice, surface, &presentCount, nullptr);

	modes.resize(presentCount);
	vkGetPhysicalDeviceSurfacePresentModesKHR(p_vkDevice->physDevice, surface, &presentCount, modes.data());

	// make sure that we have suitable swap chain extensions available before continuing
	if (formats.empty() || modes.empty())
	{
		g_filelog->WriteLog("Critcal error! Unable to locate suitable swap chains on device.");
		exit(EXIT_FAILURE);
	}

	// Next step is to determine the surface format. Ideally undefined format is preffered so we can set our own, otherwise
	// we will go with one that suits our colour needs - i.e. 8bitBGRA and SRGB.
	if ((formats.size() > 0) && (formats[0].format == VK_FORMAT_UNDEFINED))
	{
		surfaceInfo.format.format = VK_FORMAT_B8G8R8A8_UNORM;
		surfaceInfo.format.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}
	else
	{
		for (auto& format : formats)
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
			{
				surfaceInfo.format = format;
				break;
			}
	}

	// And then the presentation format - the preferred format is triple buffering
	for (auto& mode : modes)
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)													// our preferred triple buffering mode
		{
			surfaceInfo.mode = mode;
			break;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
		{
			surfaceInfo.mode = mode;																	// immediate mode is only supported on some drivers.
			break;
		}

	// Finally set the resoultion of the swap chain buffers
	// First of check if we can manually set the dimension - some GPUs allow this by setting the max as the size of uint32
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		surfaceInfo.extent = capabilities.currentExtent;									// go with the automatic settings
	else
	{
		surfaceInfo.extent = { screenWidth, screenHeight };
		surfaceInfo.extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, surfaceInfo.extent.width));
		surfaceInfo.extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, surfaceInfo.extent.height));
	}

	// Get the number of possible images we can send to the queue
	imageCount = capabilities.minImageCount + 1;								 // adding one as we would like to implement triple buffering
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		imageCount = capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceInfo.format.format;
	createInfo.imageColorSpace = surfaceInfo.format.colorSpace;
	createInfo.imageExtent = surfaceInfo.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = surfaceInfo.mode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	// if the graphics and presentation aren't the same then use concurrent sharing mode
	uint32_t queueIndicies[] = { p_vkDevice->queue.graphIndex, p_vkDevice->queue.presentIndex };

	if (p_vkDevice->queue.graphIndex != p_vkDevice->queue.presentIndex)
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
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data());
}

void VkSwapChain::PrepareImageViews()
{
	for (int c = 0; c < imageCount; ++c)
	{
		VkImageView imageView = VulkanUtility::InitImageView(images[c], surfaceInfo.format.format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, device);
		imageViews.push_back(imageView);
	}
}

void VkSwapChain::PrepareSemaphores()
{
	
	semaphore.image = VulkanUtility::CreateSemaphore(device);
	semaphore.render = VulkanUtility::CreateSemaphore(device);
}

void VkSwapChain::Init(VulkanDevice *p_vkDevice, VkInstance instance, uint32_t screenWidth, uint32_t screenHeight)
{
	PrepareSwapChain(p_vkDevice, instance, screenWidth, screenHeight);
	PrepareImageViews();
	PrepareSemaphores();
}

void VkSwapChain::Destroy()
{
	for (int c = 0; c < imageViews.size(); ++c)
		vkDestroyImageView(device, imageViews[c], nullptr);

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}