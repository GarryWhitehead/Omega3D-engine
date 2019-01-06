#include "BufferManager.h"
#include "Utility/logger.h"

namespace VulkanAPI
{

	BufferManager::BufferManager()
	{
	}


	BufferManager::~BufferManager()
	{
	}

	void BufferManager::allocate_segment(std::unique_ptr<MemoryAllocator>& mem_alloc, BufferType buff_type, BufferMemoryType mem_type, uint64_t block_size)
	{
		BufferSegmentInfo segment;
		
		segment.size = block_size;
		segment.buffer_type = buff_type;

		switch (mem_type) {
		case BufferMemoryType::Local:
			segment.buffer = mem_alloc->allocateSegment(MemoryUsage::VK_BUFFER_STATIC, block_size);
			break;
		case BufferMemoryType::Host:
			segment.buffer = mem_alloc->allocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, block_size);
			break;
		}
		
		// if a segment for the memory type already exsists, check to see if there is room in the current segment. If there isn't, create a new segment
		if (buffer_segments.find(mem_type) != buffer_segments.end()) {

			auto& old_segment = buffer_segments[mem_type];
			
		

			segment.buffer.map(device, old_segment.buffer.)

		}
		buffer_segments[mem_type] = segment;
	}

	void BufferManager::add_buffer(Buffer& buffer, BufferMemoryType mem_type, uint64_t size)
	{
		// check whether the buffer is alreay init and we are updating

		// otherwise create a new buffer
		auto& segment = buffer_segments[mem_type];
	}
}
