#pragma once

#include "VkRenderManager.h"

namespace VulkanGlobal
{
	struct VkCurrent
	{
		VkInstance instance;
		VkSurfaceKHR surface;
		VkDevice device;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceFeatures features;

		// Queue info
		VkQueue graphQueue;
		VkQueue presentQueue;
		VkQueue computeQueue;
		int32_t graphIndex;
		int32_t presentIndex;
		int32_t computeIndex;
	};

	static VkCurrent vkCurrent;

	void init_vkInstance();

	void init_windowSurface(GLFWwindow* window);

	void init_Device();

}