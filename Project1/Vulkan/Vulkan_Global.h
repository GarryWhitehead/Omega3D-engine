#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	// forward decleartions
	class MemoryAllocator;
	class BufferManager;

	namespace Global
	{
		
		struct VkManagers 
		{
			MemoryAllocator* mem_allocator;
			BufferManager* buff_manager;
		};

		static VkManagers vk_managers;

		// initilisation functions
		void init_memory_allocator(vk::Device dev, vk::PhysicalDevice gpu);
		void init_buffer_manager();
	}

}

