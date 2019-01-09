#include "Vulkan_Global.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/BufferManager.h"

namespace VulkanAPI
{
	namespace Global
	{
		void init_memory_allocator(vk::Device dev, vk::PhysicalDevice gpu)
		{
			vk_managers.mem_allocator = new MemoryAllocator(dev, gpu);
			assert(vk_managers.mem_allocator);
		}

		void init_buffer_manager()
		{
			vk_managers.buff_manager = new BufferManager();
			assert(vk_managers.buff_manager);
		}
		
	}
}
