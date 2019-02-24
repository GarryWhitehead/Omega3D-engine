#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/Queue.h"
#include <vector>
#include <set>

namespace VulkanAPI
{
	namespace Util
	{
		vk::Format find_supported_format(std::vector<vk::Format>& formats, vk::ImageTiling tiling, vk::FormatFeatureFlags format_feature, vk::PhysicalDevice& gpu);
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
			bool has_physical_device_props2 = false;
			bool has_external_capabilities = false;
			bool has_debug_utils = false;
		};


		Device();
		~Device();

		static bool find_ext_properties(const char* name, std::vector<vk::ExtensionProperties>& properties);
		static vk::Format get_depth_format(vk::PhysicalDevice& gpu);
			
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
			return surface;
		}

		uint32_t getQueueIndex(QueueType type) const;
		VulkanAPI::Queue getQueue(QueueType type);

		void set_window_surface(vk::SurfaceKHR surface, SurfaceType type = SurfaceType::SurfaceKHR)
		{
			assert(surface);
			win_surface = surface;
			surface_type = type;
		}

	private:

		vk::Instance instance;

		vk::SurfaceKHR surface;
		vk::Device device;
		vk::PhysicalDevice physical;
		vk::PhysicalDeviceFeatures features;

		struct QueueInfo
		{
			int computeIndex = -1;
			int presentIndex = -1;
			int graphIndex = -1;
			VulkanAPI::Queue graphQueue;
			VulkanAPI::Queue presentQueue;
			VulkanAPI::Queue computeQueue;
		} queue;

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
		Extensions device_ext;

		// validation layers
		std::vector<const char*> req_layers;

		// the window surface that is linked to this device
		SurfaceType surface_type;
		vk::SurfaceKHR win_surface;

#ifdef VULKAN_VALIDATION_DEBUG

		vk::DebugReportCallbackEXT debug_callback;
		vk::DebugUtilsMessengerEXT debug_messenger;

#endif

	};

}

