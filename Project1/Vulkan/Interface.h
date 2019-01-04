#pragma once

#include "volk.h"
#include <string>
#include <memory>

#include "Vulkan/Swapchain.h"

namespace VulkanAPI
{
	//forward decleartions
	class MemoryAllocator;

	class Interface
	{

	public:

		Interface(VulkanAPI::Device device, uint32_t win_width, uint32_t win_height);
		~Interface();

		std::unique_ptr<MemoryAllocator>& get_mem_alloc()
		{
			return mem_allocator;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;
		vk::Queue graphics_queue;
		vk::Queue present_queue;
		vk::Queue compute_queue;

		VulkanAPI::Swapchain swapchain_khr;

		std::unique_ptr<MemoryAllocator> mem_allocator;
	};

}

