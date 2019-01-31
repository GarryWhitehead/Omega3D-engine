#include "Vulkan_Global.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/SemaphoreManager.h"

namespace VulkanAPI
{
	namespace Global
	{
		namespace Managers
		{
			void init_memory_allocator(vk::Device dev, vk::PhysicalDevice gpu)
			{
				mem_allocator.init(dev, gpu);
			}

			void init_semaphore_manager(vk::Device dev)
			{
				semaphore_manager.init(dev);
			}

		}
	}
}
