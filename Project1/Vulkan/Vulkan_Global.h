#pragma once

#include "VkRenderManager.h"

namespace VulkanGlobal
{
	
	// defines the parameters for the current vulkan device. At the moment, only one GPU is supported
	struct VkCurrent
	{
		VkInstance instance;
		VkSurfaceKHR surface;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceFeatures features;

		int computeIndex = -1;
		int presentIndex = -1; 
		int graphIndex = -1;
		VkQueue graphQueue;
		VkQueue presentQueue;
		VkQueue computeQueue;

		// info on this device's swapchain
		VkSurfaceFormatKHR format;
		VkPresentModeKHR mode;
		VkExtent2D extent;
		VkSwapchainKHR swapchain;

		std::vector<VkImageView> imageViews;

		// and syncig semaphores for the swapchain
		VkSemaphore imageSemaphore;
		VkSemaphore presentSemaphore;
	};

	static VkCurrent vkCurrent;

	void init_vkInstance();

	void init_windowSurface(GLFWwindow* window);

	void init_device();

	void init_swapchain(uint32_t width, uint32_t height);


	// the globally represented managers for vulkan
	struct VkManagers
	{
		VkMemoryManager* vkMemoryManager;
	};

	static VkManagers vkManagers;

	// init functions for managers
	void init_vkMemoryManager();
}