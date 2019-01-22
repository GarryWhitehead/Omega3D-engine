#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	// forward decleartions
	class MemoryAllocator;
	class BufferManager; 
	class SemaphoreManager;

	namespace Global
	{
		
		namespace Managers 
		{
			MemoryAllocator mem_allocator;
			SemaphoreManager semaphore_manager;
			BufferManager* buff_manager;

			// initilisation functions
			void init_memory_allocator(vk::Device dev, vk::PhysicalDevice gpu);
			void init_semaphore_manager(vk::Device dev);
			void init_buffer_manager();
		}

		
	}

}

