#pragma once

#include "VkRenderManager.h"

namespace VulkanGlobal
{
	
	// defines the parameters for the current vulkan device. At the moment, only one GPU is supported
	struct VkCurrent
	{
		
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