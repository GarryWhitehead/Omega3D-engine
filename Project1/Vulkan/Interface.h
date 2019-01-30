#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/Queue.h"

#include <string>
#include <memory>

namespace VulkanAPI
{

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

		vk::Semaphore& get_swapchain_semaphore()
		{
			return graphics_semaphore;
		}

		vk::Semaphore& get_present_semaphore()
		{
			return present_semaphore;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		Queue graphics_queue;
		Queue present_queue;
		Queue compute_queue;

		// semaphore used for graphics and presentation queues
		vk::Semaphore graphics_semaphore;
		vk::Semaphore present_semaphore;

		VulkanAPI::Swapchain swapchain_khr;
	};

}

