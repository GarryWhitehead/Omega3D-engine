#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

namespace VulkanUtil
{
vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu);
}

/**
 * The current vulkan instance. Encapsulates all information extracted from the device
 * and physical device. Makes passing vulkan information around easier.
 *
 */
class VkContext
{
public:
	enum class QueueType
	{
		Graphics,
		Present,
		Compute,
		Count
	};

	struct Extensions
	{
		bool hasPhysicalDeviceProps2 = false;
		bool hasExternalCapabilities = false;
		bool hasDebugUtils = false;
	};

	static bool findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);
	static vk::Format getDepthFormat(vk::PhysicalDevice& gpu);

	static void createInstance(const char** glfwExtension, uint32_t extCount);
	void prepareDevice();

	friend class VkDriver;

private:
	vk::Instance instance;

	vk::Device device;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceFeatures features;

	struct QueueInfo
	{
		int compute = VK_QUEUE_FAMILY_IGNORED;
		int present = VK_QUEUE_FAMILY_IGNORED;
		int graphics = VK_QUEUE_FAMILY_IGNORED;
	} queueFamilyIndex;

	VulkanAPI::Queue graphicsQueue;
	VulkanAPI::Queue presentQueue;
	VulkanAPI::Queue computeQueue;

	// and syncing semaphores for the swapchain
	vk::Semaphore imageSemaphore;
	vk::Semaphore presentSemaphore;

	// supported extensions
	Extensions deviceExtensions;

	// validation layers
	std::vector<const char*> requiredLayers;
};


}    // namespace VulaknAPI
