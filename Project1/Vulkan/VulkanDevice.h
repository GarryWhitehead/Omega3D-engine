#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

namespace VkInitilisation
{

	// functions
	VkDevice PrepareDevice(VkPhysicalDevice physDevice, int graphIndex, int presentIndex, int computeIndex);
	bool FindDeviceExtenisions(VkPhysicalDevice physDevice, const char* reqDevice);
	VkPhysicalDeviceFeatures getPhysicalDeviceFeatures(VkPhysicalDevice physDevice);
	VkPhysicalDevice PreparePhysicalDevice(const VkInstance instance);
	void PrepareQueueIndices(VkSurfaceKHR surface, VkPhysicalDevice physDevice, int& graphIndex, int& presentIndex, int& computeIndex);
	void PrepareQueues(VkDevice device, int graphIndex, int presentIndex, int computeIndex, VkQueue& graphQueue, VkQueue& presentQueue, VkQueue& computeQueue);

};

