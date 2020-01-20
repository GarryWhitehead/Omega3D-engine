#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

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

	VkContext() = default;

	static bool findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);

	bool prepareExtensions(std::vector<const char*>& extensions, uint32_t extCount, std::vector<vk::ExtensionProperties>& extensionProps);

	vk::PhysicalDeviceFeatures prepareFeatures();

	/**
	* @brief Creates a new abstract instance of vulkan
	*/
	bool createInstance(const char** glfwExtension, uint32_t extCount);

	/**
	* @brief Sets up all the vulkan devices and queues.
	*/
	bool prepareDevice(const vk::SurfaceKHR windowSurface);

	// ============= getters =================
    vk::Instance& getInstance();
    vk::Device& getDevice();
	vk::PhysicalDevice& getGpu();
	vk::Queue& getGraphQueue();
	vk::Queue& getPresentQueue();
	vk::Queue& getCompQueue();
	int getComputeQueueIdx() const;
	int getPresentQueueIdx() const;
	int getGraphQueueIdx() const;

	friend class VkDriver;

private:

	vk::Instance instance;

	vk::Device device;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceFeatures features;

	struct QueueInfo
	{
		uint32_t compute = VK_QUEUE_FAMILY_IGNORED;
		uint32_t present = VK_QUEUE_FAMILY_IGNORED;
		uint32_t graphics = VK_QUEUE_FAMILY_IGNORED;
	} queueFamilyIndex;

	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Queue computeQueue;

	// supported extensions
	Extensions deviceExtensions;

	// validation layers
	std::vector<const char*> requiredLayers;
    
#ifdef VULKAN_VALIDATION_DEBUG

    vk::DebugReportCallbackEXT debugCallback;
    vk::DebugUtilsMessengerEXT debugMessenger;

#endif
};


}    // namespace VulkanAPI