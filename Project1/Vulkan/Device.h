#pragma once
#include "volk.h"

#include "vulkan/vulkan.hpp"
#include <vector>
#include <set>

namespace VulkanAPI
{

	class Device
	{

		struct Extensions
		{
			bool has_physical_device_props2 = false;
			bool has_external_capabilities = false;
			bool has_debug_utils = false;
		};

	public:

		Device();
		~Device();

		void CreateInstance(const char **glfwExtension, uint32_t extCount);
		void prepareQueueIndices();
		void prepareQueues();
		void preparePhysicalDevice();
		void getPhysicalDeviceFeatures();
		void prepareDevice();

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
			vk::Queue graphQueue;
			vk::Queue presentQueue;
			vk::Queue computeQueue;
		} queue;

		// info on this device's swapchain
		struct SwapchainInfo
		{
			vk::SurfaceFormatKHR format;
			vk::PresentModeKHR mode;
			vk::Extent2D extent;
			vk::SwapchainKHR swapchain;
		} swapchain;

		std::vector<VkImageView> imageViews;

		// and syncig semaphores for the swapchain
		vk::Semaphore imageSemaphore;
		vk::Semaphore presentSemaphore;

		// extensions
		Extensions device_ext;

		std::vector<const char*> layers;
	};

}

