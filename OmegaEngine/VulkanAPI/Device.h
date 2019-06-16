#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Queue.h"
#include <vector>
#include <set>

namespace VulkanAPI
{
	namespace Util
	{
		vk::Format findSupportedFormat(std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags formatFeature, vk::PhysicalDevice& gpu);
	}

	class Device
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


		Device();
		~Device();

		static bool findExtensionProperties(const char* name, std::vector<vk::ExtensionProperties>& properties);
		static vk::Format getDepthFormat(vk::PhysicalDevice& gpu);
			
		void createInstance(const char **glfwExtension, uint32_t extCount);
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

		vk::SurfaceKHR& getSurface()
		{
			return windowSurface;
		}

		uint32_t getQueueIndex(QueueType type) const;
		VulkanAPI::Queue getQueue(QueueType type);

		void setWindowSurface(vk::SurfaceKHR& surface, SurfaceType type = SurfaceType::SurfaceKHR)
		{
			assert(surface);
			windowSurface = surface;
			surfaceType = type;
		}

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

		// info on this device's swapchain
		struct SwapchainInfo
		{
			vk::SurfaceFormatKHR format;
			vk::PresentModeKHR mode;
			vk::Extent2D extent;
			vk::SwapchainKHR swapchain;
		} swapchain;

		// the images views for rendering via the framebuffer
		std::vector<VkImageView> imageViews;

		// and syncig semaphores for the swapchain
		vk::Semaphore imageSemaphore;
		vk::Semaphore presentSemaphore;

		// supported extensions
		Extensions deviceExtensions;

		// validation layers
		std::vector<const char*> requiredLayers;

		// the window surface that is linked to this device
		SurfaceType surfaceType;
		vk::SurfaceKHR windowSurface;

#ifdef VULKAN_VALIDATION_DEBUG

		vk::DebugReportCallbackEXT debugCallback;
		vk::DebugUtilsMessengerEXT debugMessenger;

#endif

	};

}

