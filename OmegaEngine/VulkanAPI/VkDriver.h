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

/**
* @brief A wrappeer for a vulkan instance
*/
class Instance
{
public:

	// no copying allowed!
	Instance(const Instance&) = delete;
	Instance& operator=(const Instance&) = delete;
	Instance(Instance&&) = default;
	Instance& operator=(Instance&&) = default;

private:
	
	vk::Instance instance;
};

/**
 * The current vulkan instance. Encapsulates all information extracted from the device
 * and physical device. Makes passing vulkan information around easier.
 *
 */
struct VkContext
{
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

class VkDriver
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

	VkDriver();
	~VkDriver();

	static bool findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);
	static vk::Format getDepthFormat(vk::PhysicalDevice& gpu);

	static void createInstance(const char** glfwExtension, uint32_t extCount);
	void prepareDevice();

    VkContext& getContext()
    {
        return context;
    }


	// ====== manager helper functions =========
	CommandBufferManager& getCmdBufManager()
	{
		return cmdBufManager;
	}

    ProgramManager& getProgManager()
    {
        return progManager;
    }

private:
    
	// managers
    ProgamManager progManager;
	CommandBufferManager cbManager;
    
    // the current device context
    VkContext context;
    
    // resources associated with this device
    std::unordered_map<VkHandle, VkTexture>;
    std::unordered_map<VkHandle, VkBuffer>;
    
#ifdef VULKAN_VALIDATION_DEBUG

	vk::DebugReportCallbackEXT debugCallback;
	vk::DebugUtilsMessengerEXT debugMessenger;

#endif
};

}    // namespace VulkanAPI
