#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Queue.h"

#include "VulkanAPI/Managers/BufferManager.h"
#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Managers/PipelineManager.h"
#include "VulkanAPI/Managers/VkTextureManager.h"

#include <set>
#include <vector>

namespace VulkanAPI
{
namespace VulkanUtil
{
vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling,
                               vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu);
}

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

	enum class SurfaceType
	{
		SurfaceKHR,
		None
	};

	struct Extensions
	{
		bool hasPhysicalDeviceProps2 = false;
		bool hasExternalCapabilities = false;
		bool hasDebugUtils = false;
	};

	VkContext();
	~VkContext();

	static bool findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);
	static vk::Format getDepthFormat(vk::PhysicalDevice& gpu);

	static void createInstance(const char** glfwExtension, uint32_t extCount);
	void prepareDevice();

	vk::Instance& getInstance()
	{
		return instance;
	}

	vk::Device& getDevice()
	{
		return device;
	}

	vk::PhysicalDevice& getPhysicalDevice()
	{
		return physical;
	}

	uint32_t getQueueIndex(QueueType type) const;
	VulkanAPI::Queue getQueue(QueueType type);

	// ====== manager helper functions =========
	PipelineManager& getPLineManager()
	{
		return plineManager;
	}

	BufferManager& getBufManager()
	{
		return bufManager;
	}

	CommandBufferManager& getCmdBufManager()
	{
		return cmdBufManager;
	}

	VkTextureManager& getTexManager()
	{
		return texManager;
	}

private:
	// managers
	PipelineManager plineManager;
	CommandBufferManager cmdBufManager;
	BufferManager bufManager;
	VkTextureManager texManager;

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

	// the images views for rendering via the framebuffer
	std::vector<VkImageView> imageViews;

	// and syncig semaphores for the swapchain
	vk::Semaphore imageSemaphore;
	vk::Semaphore presentSemaphore;

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
