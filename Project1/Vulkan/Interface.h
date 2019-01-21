#pragma once

#include "volk.h"
#include <string>
#include <memory>

#include "Vulkan/Swapchain.h"
#include "Vulkan/Queue.h"

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

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		Queue graphics_queue;
		Queue present_queue;
		Queue compute_queue;

		VulkanAPI::Swapchain swapchain_khr;
	};

}

