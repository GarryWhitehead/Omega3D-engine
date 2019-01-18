#pragma once

#include "volk.h"
#include <string>
#include <memory>

#include "Vulkan/Swapchain.h"

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

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		vk::Queue graphics_queue;
		vk::Queue present_queue;
		vk::Queue compute_queue;

		VulkanAPI::Swapchain swapchain_khr;
	};

}

