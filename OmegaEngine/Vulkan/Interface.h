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

		Swapchain& get_swapchain()
		{
			return swapchain_khr;
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
	};

}

