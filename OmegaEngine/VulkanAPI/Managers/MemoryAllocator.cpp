#include "MemoryAllocator.h"
#include "Utility/logger.h"
#include "VulkanAPI/CommandBuffer.h"

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
	graphicsQueue = queue;
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

void MemoryAllocator::createBuffer(uint32_t size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props,
                                   vk::DeviceMemory& memory, vk::Buffer& buffer)
{
	vk::BufferCreateInfo createInfo({}, size, flags, vk::SharingMode::eExclusive);

	VK_CHECK_RESULT(device.createBuffer(&createInfo, nullptr, &buffer));

	vk::MemoryRequirements memoryReq;
	device.getBufferMemoryRequirements(buffer, &memoryReq);

	uint32_t memoryType = findMemoryType(memoryReq.memoryTypeBits, props);

	vk::MemoryAllocateInfo memoryInfo(memoryReq.size, memoryType);

	VK_CHECK_RESULT(device.allocateMemory(&memoryInfo, nullptr, &memory));
	device.bindBufferMemory(buffer, memory, 0);
}

vk::Buffer& MemoryAllocator::getMemoryBuffer(const uint32_t id)
{
	assert(id < memoryBlocks.size());
	return memoryBlocks[id].blockBuffer;
}

vk::DeviceMemory& MemoryAllocator::getDeviceMemory(const uint32_t id)
{
	assert(id < memoryBlocks.size());
	return memoryBlocks[id].blockMemory;
}

uint32_t MemoryAllocator::allocateBlock(MemoryType type, uint32_t size)
{
	// default block allocation is as follows:
	// host visisble buffers are allocated with a default page size of 64mb and are intened for dynamic data handling
	// local buffers are allocated with a default page size of 256mb and default to storage, index and vertex buffers were data is static
	MemoryBlock block;
	uint32_t allocatedSize = 0;

	if (type == MemoryType::VK_BLOCK_TYPE_HOST)
	{
		allocatedSize = (size == 0) ? static_cast<uint32_t>(ALLOC_BLOCK_SIZE_HOST) : size;

		// instead of creating a buffer for each usage flag, just allow this buffer to hold any usage type. This keeps allocations to a min and doesn't seem to effect performance
		// TODO: remove the usage parameter as not really needed
		createBuffer(allocatedSize,
		             vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eVertexBuffer |
		                 vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eStorageBuffer |
		                 vk::BufferUsageFlagBits::eTransferDst,
		             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		             block.blockMemory, block.blockBuffer);

		block.type = MemoryType::VK_BLOCK_TYPE_HOST;
	}
	else if (type == MemoryType::VK_BLOCK_TYPE_LOCAL)
	{
		allocatedSize = (size == 0) ? static_cast<uint32_t>(ALLOC_BLOCK_SIZE_LOCAL) : size;

		// locally created buffers have the transfer dest bit as the data will be copied from a temporary hosted buffer to the local destination buffer
		createBuffer(allocatedSize,
		             vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer |
		                 vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eUniformBuffer |
		                 vk::BufferUsageFlagBits::eStorageBuffer,
		             vk::MemoryPropertyFlagBits::eDeviceLocal, block.blockMemory, block.blockBuffer);

		block.type = MemoryType::VK_BLOCK_TYPE_LOCAL;
	}

	block.blockId = static_cast<int32_t>(memoryBlocks.size());
	block.totalSize = allocatedSize;
	memoryBlocks.push_back(block);

	return block.blockId;
}

uint32_t MemoryAllocator::allocateBlock(MemoryUsage memoryUsage)
{
	uint32_t blockId;

	if (memoryUsage & MemoryUsage::VK_BUFFER_DYNAMIC)
	{
		blockId = allocateBlock(MemoryType::VK_BLOCK_TYPE_HOST, static_cast<uint32_t>(ALLOC_BLOCK_SIZE_HOST));
	}
	else if (memoryUsage & MemoryUsage::VK_BUFFER_STATIC)
	{
		blockId = allocateBlock(MemoryType::VK_BLOCK_TYPE_LOCAL, static_cast<uint32_t>(ALLOC_BLOCK_SIZE_LOCAL));
	}

	return blockId;
}

MemorySegment MemoryAllocator::allocate(MemoryUsage memoryUsage, uint32_t size)
{
	assert(device);
	assert(gpu);

	uint32_t blockId = 0;

	// Ensure that the user has already alloacted a block of memory. If not, then allocate for them using the default parameters for memory type and size
	if (memoryBlocks.empty())
	{
		blockId = allocateBlock(memoryUsage);
	}
	else
	{
		blockId = findBlockType(memoryUsage);
		if (blockId == UINT32_MAX)
		{
			blockId = allocateBlock(memoryUsage);
		}
	}

	// Vulkan expects buffers to be aligned to a minimum buffer size which is set by the specification and at present is 256bytes
	// segments which are smaller than this value will be adjusted accordingly and aligned
	vk::PhysicalDeviceProperties properties = gpu.getProperties();

	vk::DeviceSize minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
	uint32_t segmentSize = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

	// ensure that there is enough free space in this particular block to accomodate the data segment
	uint32_t offset = findFreeSegment(blockId, segmentSize);

	// if error code returned, allocate another block of memory of the required type
	if (offset == UINT32_MAX)
	{
		blockId = allocateBlock(memoryUsage);
		offset = findFreeSegment(blockId, segmentSize);
	}

	return { blockId, offset, segmentSize };
}

uint32_t MemoryAllocator::findBlockType(MemoryUsage usage)
{
	uint32_t id = UINT32_MAX;

	for (auto& block : memoryBlocks)
	{
		if (block.type == MemoryType::VK_BLOCK_TYPE_HOST && (usage & MemoryUsage::VK_BUFFER_DYNAMIC))
		{
			id = block.blockId;
		}
		else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL && (usage & MemoryUsage::VK_BUFFER_STATIC))
		{
			id = block.blockId;
		}
	}
	return id;
}

uint32_t MemoryAllocator::findFreeSegment(const uint32_t id, const uint32_t segmentSize)
{
	if (segmentSize > memoryBlocks[id].totalSize)
	{
		return UINT32_MAX;
	}

	uint32_t offset = UINT32_MAX;

	// first check whether the block has been segmented - if not, then life is easier as we can just go ahead and allocate a segment
	if (memoryBlocks[id].freeSegments.empty())
	{
		// add to the allocated pool
		offset = 0;
		memoryBlocks[id].allocatedSegments.insert(std::make_pair(offset, segmentSize));

		// add segment with available space remaining in block
		memoryBlocks[id].freeSegments.insert(
		    std::make_pair(offset + segmentSize, memoryBlocks[id].totalSize - segmentSize));
	}
	else
	{
		// find segment with required size
		for (auto segment : memoryBlocks[id].freeSegments)
		{
			if (segment.second >= segmentSize)
			{
				// first = offset; second = size
				offset = segment.first;
				assert(offset + segmentSize < memoryBlocks[id].totalSize);

				// add to the allocated segments pool
				memoryBlocks[id].allocatedSegments.insert(std::make_pair(offset, segmentSize));

				// if there is any free space in the this segment, add it to the free segment pool
				uint32_t unallocatedSize = segment.second - segmentSize;
				if (unallocatedSize > 0)
				{
					memoryBlocks[id].freeSegments.insert(std::make_pair(offset + segmentSize, unallocatedSize));

					// defrag mem by checking for adjacent free segments
					// DefragBlockMem(blockId);		// TODO: add this function!
				}

				// and remember to remove used segment from the free pool
				memoryBlocks[id].freeSegments.erase(offset);
				break;
			}
		}
	}

	return offset;
}

// override of the segment mapping function that allows data of type *void to be passed
void MemoryAllocator::mapDataToSegment(MemorySegment& segment, void* data, uint32_t totalSize, uint32_t offset)
{
	assert(segment.getId() < (int32_t)memoryBlocks.size());
	MemoryBlock block = memoryBlocks[segment.getId()];

	// How we map the data depends on the memory type;
	// For host-visible memory, the memory is mapped and the data is just mem-copied across
	// With device local, we must first create a buffer on the host, map the data to that and then copyCmd it across to local memory
	if (block.type == MemoryType::VK_BLOCK_TYPE_HOST)
	{
		segment.map(device, block.blockMemory, segment.getOffset(), data, totalSize, offset);
	}
	else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL)
	{
		// start by creating host-visible buffers
		vk::Buffer tempBuffer;
		vk::DeviceMemory tempMemory;
		createBuffer(segment.getSize(), vk::BufferUsageFlagBits::eTransferSrc,
		             vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, tempMemory,
		             tempBuffer);

		// map data to temporary buffer
		segment.map(device, tempMemory, 0, data, totalSize,
		            offset);    // mem offseting not allowed for device local buffer

		// create cmd buffer for copy and transfer to device local memory
		CommandBuffer copyCmdBuffer(device, graphicsQueue.getIndex());
		copyCmdBuffer.createPrimary();

		vk::BufferCopy bufferCopy{ 0, segment.getOffset(), segment.getSize() };
		copyCmdBuffer.get().copyBuffer(tempBuffer, block.blockBuffer, 1, &bufferCopy);
		copyCmdBuffer.end();
		graphicsQueue.flushCmdBuffer(copyCmdBuffer.get());

		// clear up and we are done
		device.destroyBuffer(tempBuffer, nullptr);
		device.freeMemory(tempMemory, nullptr);
	}
}

void MemoryAllocator::destroySegment(MemorySegment& segment)
{
	if (segment.getSize() <= 0)
	{    // if the segment size is zero, it obviously hasn't yet been allocated
		return;
	}

	// check that the segment exsists within the map
	auto& allocatedSegments = memoryBlocks[segment.getId()].allocatedSegments;
	auto& iter = allocatedSegments.find(segment.getOffset());

	if (iter != allocatedSegments.end())
	{

		// remove segment from the allocated list
		allocatedSegments.erase(segment.getOffset());

		// and add to the free segments pool
		memoryBlocks[segment.getId()].freeSegments.insert(std::make_pair(segment.getOffset(), segment.getSize()));
	}
}

void MemoryAllocator::destroyBlock(uint32_t id)
{
	if (id < memoryBlocks.size())
	{

		// handle the vulkan side first
		device.destroyBuffer(memoryBlocks[id].blockBuffer, nullptr);
		device.freeMemory(memoryBlocks[id].blockMemory, nullptr);

		// and remove from the memory block pool
		memoryBlocks.erase(memoryBlocks.begin() + (id + 1));
	}
}

void MemoryAllocator::destroyAllBlocks()
{
	for (auto& block : memoryBlocks)
	{

		destroyBlock(block.blockId);
	}
}

void MemoryAllocator::outputLog()
{
	// just outputting to stderr for now. Would be useful to write to csv file at some point
	LOGGER_INFO("Current memory blocks in use: %zu", memoryBlocks.size());

	for (auto& block : memoryBlocks)
	{
		LOGGER_INFO("-----------------------------------------\n");
		LOGGER_INFO("Block Id #%i :\n", block.blockId);
		LOGGER_INFO("Total size = %i\n", block.totalSize);
		LOGGER_INFO("Number of allocated segments: %zu\n", block.allocatedSegments.size());

		LOGGER("Allocated segments.....\n");
		uint32_t count = 0;
		uint32_t totalUsed = 0;

		for (auto& segment : block.allocatedSegments)
		{
			LOGGER_INFO("Segment %i:   Offset = %i     Size = %i\n", count, segment.first, segment.second);
			++count;
			totalUsed += segment.second;
		}
		LOGGER_INFO("Total memory used = %i\n", totalUsed);
		LOGGER_INFO("Memory available: %i\n", block.totalSize - totalUsed);

		LOGGER("Free segments.....\n");
		count = 0;
		totalUsed = 0;
		for (auto& segment : block.freeSegments)
		{
			LOGGER_INFO("Segment %i:   Offset = %i     Size = %i\n", count, segment.first, segment.second);
			++count;
			totalUsed += segment.second;
		}
		LOGGER_INFO("Total free memory available: %i\n", totalUsed);
	}
}

// ================================================================ MemorySegment functions ====================================================================================================================

void MemorySegment::map(vk::Device dev, vk::DeviceMemory memory, const uint32_t offset, void* sourceData,
                        uint32_t totalSize, uint32_t mappedOffset)
{
	assert(totalSize <= size);

	if (data == nullptr)
	{

		VK_CHECK_RESULT(dev.mapMemory(memory, offset, size, (vk::MemoryMapFlags)0, &data));
		memcpy(data, sourceData, totalSize);
		dev.unmapMemory(memory);
	}
	else
	{
		// preserve the original mapped loaction - cast to char* required as unable to add offset to incomplete type such as void*
		void* mapped = static_cast<char*>(data) + mappedOffset;
		memcpy(mapped, sourceData, totalSize);
	}
}
}    // namespace VulkanAPI