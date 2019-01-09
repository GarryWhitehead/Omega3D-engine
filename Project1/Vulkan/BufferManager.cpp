#include "BufferManager.h"
#include "Utility/logger.h"
#include "Vulkan/Vulkan_Global.h"
#include "Vulkan/Buffer.h"

namespace VulkanAPI
{

	BufferManager::BufferManager()
	{
	}


	BufferManager::~BufferManager()
	{
	}

	void BufferManager::allocate_segment(BufferType buff_type, BufferMemoryType mem_type, uint64_t block_size)
	{
		BufferSegmentInfo segment;
		
		segment.size = block_size;
		segment.buffer_type = buff_type;

		switch (mem_type) {
		case BufferMemoryType::Local:
			segment.buffer = Global::vk_managers.mem_allocator->allocateSegment(MemoryUsage::VK_BUFFER_STATIC, block_size);
			break;
		case BufferMemoryType::Host:
			segment.buffer = Global::vk_managers.mem_allocator->allocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, block_size);
			break;
		}
		
		// if a segment for the memory type already exsists, check to see if there is room in the current segment. If there isn't, create a new segment
		if (buffer_segments.find(mem_type) != buffer_segments.end()) {

			auto& old_segment = buffer_segments[mem_type];
			
		

			segment.buffer.map(device, old_segment.buffer.)

		}
		buffer_segments[mem_type] = segment;
	}

	Buffer BufferManager::create(uint64_t size)
	{
		Buffer& buffer;

		// check size and whether its under the vulkan struct size limit

		// check whether the buffer is alreay init and we are updating

		// otherwise create a new buffer
		auto& segment = buffer_segments[mem_type];
	}
}
