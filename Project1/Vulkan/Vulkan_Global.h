#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/SemaphoreManager.h"

namespace VulkanAPI
{
	// forward decleartions


	namespace Global
	{
		
		namespace Managers 
		{
			extern MemoryAllocator mem_allocator;
			extern SemaphoreManager semaphore_manager;

			// initilisation functions
			void init_memory_allocator(vk::Device dev, vk::PhysicalDevice gpu);
			void init_semaphore_manager(vk::Device dev);
		}

		
	}

}

