#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

struct VulkanDevice
{

	struct QueueInfo
	{
		QueueInfo() : graphIndex(-1), presentIndex(-1) {}

		VkQueue graphQueue;
		VkQueue presentQueue;
		VkQueue computeQueue;
		int32_t graphIndex;
		int32_t presentIndex;
		int32_t computeIndex;
	} queue;

	VulkanDevice();
	~VulkanDevice();

	void Init(VkInstance instance, VkSurfaceKHR surface);
	void PrepareDevice();
	void PreparePhysicalDevice(const VkInstance instance);
	void PrepareQueues(VkSurfaceKHR surface);
	bool FindDeviceExtenisions(VkPhysicalDevice physDevice, const char* reqDevice);

	// GPU device handle
	VkPhysicalDevice physDevice;

	// GPU features
	VkPhysicalDeviceFeatures features;
	VkDevice device;

	std::vector<const char*> deviceExt;
};

