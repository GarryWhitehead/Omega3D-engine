#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

namespace VkInitilisation
{
	// functions 
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
		uint32_t screenHeight);

	std::vector<VkImageView> PrepareImageViews(VkDevice device, std::vector<VkImage>& images, VkSurfaceFormatKHR format);
	void PrepareSemaphores(VkDevice device, VkSemaphore image, VkSemaphore present);
	void DestroySwapChain(VkDevice device, VkSwapchainKHR swapChain, std::vector<VkImageView> imageViews);
}

