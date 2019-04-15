#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/Queue.h"

#include <string>
#include <memory>

namespace VulkanAPI
{
	// forward declerations
	class BufferManager;
	class SemaphoreManager;
	class VkTextureManager;

	class Interface
	{

	public:

		Interface(VulkanAPI::Device device, uint32_t win_width, uint32_t win_height);
		~Interface();

		vk::Device& get_device()
		{
			return device;
		}

		vk::PhysicalDevice& get_gpu()
		{
			return gpu;
		}

		Queue& get_graph_queue()
		{
			return graphics_queue;
		}

		Queue& get_present_queue()
		{
			return present_queue;
		}

		Swapchain& get_swapchain()
		{
			return swapchain_khr;
		}

		std::unique_ptr<BufferManager>& get_buffer_manager()
		{
			return buffer_manager;
		}

		std::unique_ptr<SemaphoreManager>& get_semaphore_manager()
		{
			return semaphore_manager;
		}

		std::unique_ptr<VkTextureManager>& get_texture_manager()
		{
			return texture_manager;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		// queues associated with this device
		Queue graphics_queue;
		Queue present_queue;
		Queue compute_queue;

		// the swap-chain
		VulkanAPI::Swapchain swapchain_khr;

		// managers
		std::unique_ptr<BufferManager> buffer_manager;
		std::unique_ptr<SemaphoreManager> semaphore_manager;
		std::unique_ptr<VkTextureManager> texture_manager;
	};

}

