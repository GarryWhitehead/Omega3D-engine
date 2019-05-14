#include "MemoryAllocator.h"
#include "Vulkan/CommandBuffer.h"
#include "Utility/logger.h"

namespace VulkanAPI
{

	MemoryAllocator::MemoryAllocator()
	{
	}

	MemoryAllocator::MemoryAllocator(vk::Device& dev, vk::PhysicalDevice& physical, Queue& queue)
	{
		init(dev, physical, queue);
	}

	MemoryAllocator::~MemoryAllocator()
	{
		destroyAllBlocks();
	}

	void MemoryAllocator::init(vk::Device& dev, vk::PhysicalDevice& physical, Queue& queue)
	{
		device = dev;
		gpu = physical;
		graph_queue = queue;
	}

	uint32_t MemoryAllocator::findMemoryType(uint32_t type, vk::MemoryPropertyFlags flags)
	{
		vk::PhysicalDeviceMemoryProperties memoryProp = gpu.getMemoryProperties();

		for (uint32_t c = 0; c < memoryProp.memoryTypeCount; ++c)
		{
			if ((type & (1 << c)) && (memoryProp.memoryTypes[c].propertyFlags & flags) == flags)
				return c;
		}

		// no requested memory type found so return with error
		return UINT32_MAX;
	}

	void MemoryAllocator::createBuffer(uint32_t size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props, vk::DeviceMemory& memory, vk::Buffer& buffer)
	{
		vk::BufferCreateInfo createInfo({}, size, flags, vk::SharingMode::eExclusive);

		VK_CHECK_RESULT(device.createBuffer(&createInfo, nullptr, &buffer));

		vk::MemoryRequirements memoryReq;
		device.getBufferMemoryRequirements(buffer, &memoryReq);

		uint32_t mem_type = findMemoryType(memoryReq.memoryTypeBits, props);

		vk::MemoryAllocateInfo memoryInfo(memoryReq.size, mem_type);

		VK_CHECK_RESULT(device.allocateMemory(&memoryInfo, nullptr, &memory));
		device.bindBufferMemory(buffer, memory, 0);
	}

	vk::Buffer& MemoryAllocator::get_memory_buffer(const uint32_t id)
	{
		assert(id < mem_blocks.size());
		return mem_blocks[id].block_buffer;
	}

	vk::DeviceMemory& MemoryAllocator::get_device_memory(const uint32_t id)
	{
		assert(id < mem_blocks.size());
		return mem_blocks[id].block_mem;
	}

	uint32_t MemoryAllocator::allocateBlock(MemoryType type, uint32_t size)
	{
		// default block allocation is as follows: 
		// host visisble buffers are allocated with a default page size of 64mb and are intened for dynamic data handling
		// local buffers are allocated with a default page size of 256mb and default to storage, index and vertex buffers were data is static

		MemoryBlock block;
		uint32_t alloc_size = 0;

		if (type == MemoryType::VK_BLOCK_TYPE_HOST) {

			alloc_size = (size == 0) ? static_cast<uint32_t>(ALLOC_BLOCK_SIZE_HOST) : size;

			// instead of creating a buffer for each usage flag, just allow this buffer to hold any usage type. This keeps allocations to a min and doesn't seem to effect performance
			// TODO: remove the usage parameter as not really needed
			createBuffer(alloc_size, vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eUniformBuffer
				| vk::BufferUsageFlagBits::eStorageBuffer| vk::BufferUsageFlagBits::eTransferDst,
				vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
				block.block_mem,
				block.block_buffer);

			block.type = MemoryType::VK_BLOCK_TYPE_HOST;
		}
		else if (type == MemoryType::VK_BLOCK_TYPE_LOCAL) {

			alloc_size = (size == 0) ? static_cast<uint32_t>(ALLOC_BLOCK_SIZE_LOCAL) : size;

			// locally created buffers have the transfer dest bit as the data will be copied from a temporary hosted buffer to the local destination buffer
			createBuffer(alloc_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eUniformBuffer
				| vk::BufferUsageFlagBits::eStorageBuffer,
				vk::MemoryPropertyFlagBits::eDeviceLocal,
				block.block_mem,
				block.block_buffer);

			block.type = MemoryType::VK_BLOCK_TYPE_LOCAL;
		}

		block.block_id = static_cast<int32_t>(mem_blocks.size());
		block.total_size = alloc_size;
		mem_blocks.push_back(block);

		return block.block_id;
	}

	uint32_t MemoryAllocator::allocateBlock(MemoryUsage mem_usage)
	{
		uint32_t block_id;

		if (mem_usage & MemoryUsage::VK_BUFFER_DYNAMIC) {

			block_id = allocateBlock(MemoryType::VK_BLOCK_TYPE_HOST, static_cast<uint32_t>(ALLOC_BLOCK_SIZE_HOST));
		}
		else if (mem_usage & MemoryUsage::VK_BUFFER_STATIC) {

			block_id = allocateBlock(MemoryType::VK_BLOCK_TYPE_LOCAL, static_cast<uint32_t>(ALLOC_BLOCK_SIZE_LOCAL));
		}

		return block_id;
	}

	MemorySegment MemoryAllocator::allocate(MemoryUsage mem_usage, uint32_t size)
	{
		assert(device);
		assert(gpu);
		
		uint32_t block_id = 0;

		// Ensure that the user has already alloacted a block of memory. If not, then allocate for them using the default parameters for memory type and size
		if (mem_blocks.empty()) {

			block_id = allocateBlock(mem_usage);
		}
		else {
			block_id = findBlockType(mem_usage);
			if (block_id == UINT32_MAX) {

				block_id = allocateBlock(mem_usage);
			}
		}

		// Vulkan expects buffers to be aligned to a minimum buffer size which is set by the specification and at present is 256bytes
		// segments which are smaller than this value will be adjusted accordingly and aligned
		vk::PhysicalDeviceProperties properties = gpu.getProperties();

		vk::DeviceSize minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
		uint32_t segment_size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

		// ensure that there is enough free space in this particular block to accomodate the data segment
		uint32_t offset = findFreeSegment(block_id, segment_size);

		// if error code returned, allocate another block of memory of the required type
		if (offset == UINT32_MAX) {

			block_id = allocateBlock(mem_usage);
			offset = findFreeSegment(block_id, segment_size);
		}

		return { block_id, offset, segment_size };
	}

	uint32_t MemoryAllocator::findBlockType(MemoryUsage usage)
	{
		uint32_t id = UINT32_MAX;

		for (auto& block : mem_blocks) {

			if (block.type == MemoryType::VK_BLOCK_TYPE_HOST && (usage & MemoryUsage::VK_BUFFER_DYNAMIC)) {

				id = block.block_id;
			}
			else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL && (usage & MemoryUsage::VK_BUFFER_STATIC)) {

				id = block.block_id;
			}
		}
		return id;
	}

	uint32_t MemoryAllocator::findFreeSegment(const uint32_t id, const uint32_t segment_size)
	{
		if (segment_size > mem_blocks[id].total_size) {
			return UINT32_MAX;
		}

		uint32_t offset = UINT32_MAX;

		// first check whether the block has been segmented - if not, then life is easier as we can just go ahead and allocate a segment
		if (mem_blocks[id].free_segments.empty()) {

			// add to the allocated pool
			offset = 0;
			mem_blocks[id].alloc_segments.insert(std::make_pair(offset, segment_size));

			// add segment with available space remaining in block
			mem_blocks[id].free_segments.insert(std::make_pair(offset + segment_size, mem_blocks[id].total_size - segment_size));
		}
		else {

			// find segment with required size
			for (auto segment : mem_blocks[id].free_segments) {

				if (segment.second >= segment_size) {				// first = offset; second = size

					offset = segment.first;
					assert(offset + segment_size < mem_blocks[id].total_size);

					// add to the allocated segments pool
					mem_blocks[id].alloc_segments.insert(std::make_pair(offset, segment_size));

					// if there is any free space in the this segment, add it to the free segment pool
					uint32_t size_unalloc = segment.second - segment_size;
					if (size_unalloc > 0) {

						mem_blocks[id].free_segments.insert(std::make_pair(offset + segment_size, size_unalloc));

						// defrag mem by checking for adjacent free segments 
						// DefragBlockMem(block_id);		// TODO: add this function!
					}

					// and remember to remove used segment from the free pool
					mem_blocks[id].free_segments.erase(offset);
					break;
				}
			}
		}

		return offset;
	}

	// override of the segment mapping function that allows data of type *void to be passed
	void MemoryAllocator::mapDataToSegment(MemorySegment &segment, void *data, uint32_t totalSize, uint32_t offset)
	{
		assert(segment.get_id() < (int32_t)mem_blocks.size());
		MemoryBlock block = mem_blocks[segment.get_id()];

		// How we map the data depends on the memory type; 
		// For host-visible memory, the memory is mapped and the data is just mem-copied across
		// With device local, we must first create a buffer on the host, map the data to that and then copyCmd it across to local memory

		if (block.type == MemoryType::VK_BLOCK_TYPE_HOST) {

			segment.map(device, block.block_mem, segment.get_offset(), data, totalSize, offset);

		}
		else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL) {

			// start by creating host-visible buffers
			vk::Buffer temp_buffer;
			vk::DeviceMemory temp_memory;
			createBuffer(segment.get_size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, temp_memory, temp_buffer);

			// map data to temporary buffer
			segment.map(device, temp_memory, 0, data, totalSize, offset);		// mem offseting not allowed for device local buffer

			// create cmd buffer for copy and transfer to device local memory
			CommandBuffer copy_cmd_buff(device, graph_queue.get_index());
			copy_cmd_buff.create_primary();
			
			vk::BufferCopy buffer_copy(0, segment.get_offset(), segment.get_size());
			copy_cmd_buff.get().copyBuffer(temp_buffer, block.block_buffer, 1, &buffer_copy);
			copy_cmd_buff.end();
			graph_queue.flush_cmd_buffer(copy_cmd_buff.get());

			// clear up and we are done
			device.destroyBuffer(temp_buffer, nullptr);
			device.freeMemory(temp_memory, nullptr);
		}
	}

	void MemoryAllocator::destroySegment(MemorySegment &segment)
	{
		if (segment.get_size() <= 0) {		// if the segment size is zero, it obviously hasn't yet been allocated
			return;
		}

		// check that the segment exsists within the map
		auto & alloc_segments = mem_blocks[segment.get_id()].alloc_segments;
		auto& iter = alloc_segments.find(segment.get_offset());

		if (iter != alloc_segments.end()) {

			// remove segment from the allocated list
			alloc_segments.erase(segment.get_offset());

			// and add to the free segments pool
			mem_blocks[segment.get_id()].free_segments.insert(std::make_pair(segment.get_offset(), segment.get_size()));
		}
	}

	void MemoryAllocator::destroyBlock(uint32_t id)
	{
		if (id < mem_blocks.size()) {

			// handle the vulkan side first
			device.destroyBuffer(mem_blocks[id].block_buffer, nullptr);
			device.freeMemory(mem_blocks[id].block_mem, nullptr);

			// and remove from the memory block pool
			mem_blocks.erase(mem_blocks.begin() + (id + 1));
		}
	}

	void MemoryAllocator::destroyAllBlocks()
	{
		for (auto &block : mem_blocks) {

			destroyBlock(block.block_id);
		}
	}

	void MemoryAllocator::outputLog()
	{
		// just outputting to stderr for now. Would be useful to write to csv file at some point
		LOGGER_INFO("Current memory blocks in use: %zu", mem_blocks.size());
		
		for (auto& block : mem_blocks) {
			LOGGER_INFO("-----------------------------------------\n");
			LOGGER_INFO("Block Id #%i :\n", block.block_id);
			LOGGER_INFO("Total size = %i\n", block.total_size);
			LOGGER_INFO("Number of allocated segments: %zu\n", block.alloc_segments.size());
			
			LOGGER("Allocated segments.....\n");
			uint32_t count = 0;
			uint32_t totalUsed = 0;
			
			for (auto& segment : block.alloc_segments) {
				LOGGER_INFO("Segment %i:   Offset = %i     Size = %i\n", count, segment.first, segment.second);
				++count;
				totalUsed += segment.second;
			}
			LOGGER_INFO("Total memory used = %i\n", totalUsed);
			LOGGER_INFO("Memory available: %i\n", block.total_size - totalUsed);
			
			LOGGER("Free segments.....\n");
			count = 0;
			totalUsed = 0;
			for (auto& segment : block.free_segments) {
				LOGGER_INFO("Segment %i:   Offset = %i     Size = %i\n", count, segment.first, segment.second);
				++count;
				totalUsed += segment.second;
			}
			LOGGER_INFO("Total free memory available: %i\n", totalUsed);
		}
	}

	// ================================================================ MemorySegment functions ====================================================================================================================

	void MemorySegment::map(vk::Device dev, vk::DeviceMemory memory, const uint32_t offset, void* data_to_copy, uint32_t totalSize, uint32_t mapped_offset)
	{
		assert(totalSize <= size);

		if (data == nullptr) {

			VK_CHECK_RESULT(dev.mapMemory(memory, offset, size, (vk::MemoryMapFlags)0 , &data));
			memcpy(data, data_to_copy, totalSize);
			dev.unmapMemory(memory);
		}
		else {
			// preserve the original mapped loaction - cast to char* required as unable to add offset to incomplete type such as void*
			void *mapped = static_cast<char*>(data) + mapped_offset;		
			memcpy(mapped, data_to_copy, totalSize);
		}
	}
}