#include "VkMemoryManager.h"
#include "Vulkan_Global.h"

namespace Vulkan
{
	VkMemoryManager::VkMemoryManager()
	{
	}

	VkMemoryManager::~VkMemoryManager()
	{
		DestroyAllBlocks();
	}

	uint32_t VkMemoryManager::AllocateBlock(MemoryType type, uint32_t size)
	{
		// default block allocation is as follows: 
		// host visisble buffers are allocated with a default page size of 64mb and are intened for dynamic data handling
		// local buffers are allocated with a default page size of 256mb and default to storage, index and vertex buffers were data is static

		BlockInfo block;
		uint32_t alloc_size = 0;

		if (type == MemoryType::VK_BLOCK_TYPE_HOST) {

			alloc_size = (size == 0) ? static_cast<uint32_t>(ALLOC_BLOCK_SIZE_HOST) : size;

			CreateBuffer(alloc_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				block.block_mem,
				block.block_buffer);

			block.type = MemoryType::VK_BLOCK_TYPE_HOST;
		}
		else if (type == MemoryType::VK_BLOCK_TYPE_LOCAL) {

			alloc_size = (size == 0) ? static_cast<uint32_t>(ALLOC_BLOCK_SIZE_LOCAL) : size;

			// locally created buffers have the transfer dest bit as the data will be copied from a temporary hosted buffer to the local destination buffer
			CreateBuffer(alloc_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT |
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				block.block_mem,
				block.block_buffer);

			block.type = MemoryType::VK_BLOCK_TYPE_LOCAL;
		}

		block.block_id = mem_blocks.size();
		block.total_size = alloc_size;
		mem_blocks.push_back(block);

		return block.block_id;
	}

	uint32_t VkMemoryManager::AllocateBlock(MemoryUsage usage)
	{
		uint32_t block_id;

		if (usage & MemoryUsage::VK_BUFFER_DYNAMIC) {

			block_id = AllocateBlock(MemoryType::VK_BLOCK_TYPE_HOST);
		}
		else if (usage & MemoryUsage::VK_BUFFER_STATIC) {

			block_id = AllocateBlock(MemoryType::VK_BLOCK_TYPE_LOCAL);
		}

		return block_id;
	}

	VkMemoryManager::SegmentInfo VkMemoryManager::AllocateSegment(MemoryUsage usage, uint32_t size)
	{
		uint32_t block_id = 0;

		// Ensure that the user has already alloacted a block of memory. If not, then allocate for them using the default parameters for memory type and size
		if (mem_blocks.empty()) {

			block_id = AllocateBlock(usage);
		}
		else {
			block_id = FindBlockType(usage);
			if (block_id == UINT32_MAX) {

				block_id = AllocateBlock(usage);
			}
		}

		// Vulkan expects buffers to be aligned to a minimum buffer size which is set by the specification and at present is 256bytes
		// segments which are smaller than this value will be adjusted accordingly and aligned
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(VulkanGlobal::vkCurrent.physicalDevice, &properties);

		size_t minUboAlignment = properties.limits.minUniformBufferOffsetAlignment;
		uint32_t segment_size = (size + minUboAlignment - 1) & ~(minUboAlignment - 1);

		// ensure that there is enough free space in this particular block to accomodate the data segment
		uint32_t offset = FindFreeSegment(block_id, segment_size);

		// if error code returned, allocate another block of memory of the required type
		if (offset == UINT32_MAX) {

			block_id = AllocateBlock(usage);
			offset = FindFreeSegment(block_id, segment_size);
		}

		SegmentInfo segment;
		return segment = { block_id, offset, segment_size };
	}

	void VkMemoryManager::CreateBuffer(const uint32_t size, const VkBufferUsageFlags flags, const VkMemoryPropertyFlags props, VkDeviceMemory& memory, VkBuffer& buffer)
	{
		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = size;
		createInfo.usage = flags;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK_RESULT(vkCreateBuffer(VulkanGlobal::vkCurrent.device, &createInfo, nullptr, &buffer));

		VkMemoryRequirements memoryReq;
		vkGetBufferMemoryRequirements(VulkanGlobal::vkCurrent.device, buffer, &memoryReq);

		VkMemoryAllocateInfo memoryInfo = {};
		memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryInfo.allocationSize = memoryReq.size;
		memoryInfo.memoryTypeIndex = FindMemoryType(memoryReq.memoryTypeBits, props);

		VK_CHECK_RESULT(vkAllocateMemory(VulkanGlobal::vkCurrent.device, &memoryInfo, nullptr, &memory));

		VK_CHECK_RESULT(vkBindBufferMemory(VulkanGlobal::vkCurrent.device, buffer, memory, 0));
	}

	uint32_t VkMemoryManager::FindMemoryType(uint32_t type, VkMemoryPropertyFlags flags)
	{
		VkPhysicalDeviceMemoryProperties memoryProp;
		vkGetPhysicalDeviceMemoryProperties(VulkanGlobal::vkCurrent.physicalDevice, &memoryProp);

		for (uint32_t c = 0; c < memoryProp.memoryTypeCount; ++c)
		{
			if ((type & (1 << c)) && (memoryProp.memoryTypes[c].propertyFlags & flags) == flags)
				return c;
		}

		// no requested memory type found so return with error
		return UINT32_MAX;
	}

	uint32_t VkMemoryManager::FindBlockType(MemoryUsage usage)
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

	uint32_t VkMemoryManager::FindFreeSegment(const uint32_t id, const uint32_t segment_size)
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

	void VkMemoryManager::DefragBlockMemory(const uint32_t id)
	{
		//BlockInfo block = mem_blo
		//auto iter = 
	}

	VkBuffer& VkMemoryManager::blockBuffer(const uint32_t id)
	{
		assert(id < mem_blocks.size());

		return mem_blocks[id].block_buffer;
	}

	// override of the segment mapping function that allows data of type *void to be passed
	void VkMemoryManager::MapDataToSegment(SegmentInfo &segment, void *data, uint32_t totalSize, uint32_t offset)
	{
		assert(segment.block_id < (int32_t)mem_blocks.size());
		BlockInfo block = mem_blocks[segment.block_id];

		// How we map the data depends on the memory type; 
		// For host-visible memory, the memory is mapped and the data is just mem-copied across
		// With device local, we must first create a buffer on the host, map the data to that and then copyCmd it across to local memory

		if (block.type == MemoryType::VK_BLOCK_TYPE_HOST) {

			MapData(segment, block.block_mem, segment.offset, data, totalSize, offset);

		}
		else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL) {

			// start by creating host-visible buffers
			VkBuffer temp_buffer;
			VkDeviceMemory temp_memory;
			CreateBuffer(segment.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, temp_memory, temp_buffer);

			// map data to temporary buffer
			MapData(segment, temp_memory, 0, data, totalSize, offset);		// mem offseting not allowed for device local buffer

			// create cmd buffer for copy and transfer to device local memory

			VulkanUtility::CopyBuffer(temp_buffer, block.block_buffer, segment.size, p_vkEngine->GetCmdPool(), VulkanGlobal::vkCurrent.graphQueue, VulkanGlobal::vkCurrent.device, 0, segment.offset);

			// clear up and we are done
			vkDestroyBuffer(p_vkEngine->GetDevice(), temp_buffer, nullptr);
			vkFreeMemory(p_vkEngine->GetDevice(), temp_memory, nullptr);
		}
	}

	void VkMemoryManager::MapData(SegmentInfo &segment, VkDeviceMemory memory, const uint32_t offset, void* data, uint32_t totalSize, uint32_t mapped_offset)
	{
		assert(totalSize <= segment.size);

		if (segment.data == nullptr) {

			VK_CHECK_RESULT(vkMapMemory(VulkanGlobal::vkCurrent.device, memory, offset, segment.size, 0, &segment.data));
			memcpy(segment.data, data, totalSize);
			vkUnmapMemory(VulkanGlobal::vkCurrent.device, memory);
		}
		else {
			void *mapped = static_cast<char*>(segment.data) + mapped_offset;		// preserve the original mapped loaction - cast to char* required as unable to add offset to incomplete type such as void*
			memcpy(mapped, data, totalSize);
		}
	}

	void VkMemoryManager::DestroySegment(SegmentInfo &segment)
	{
		if (segment.size <= 0) {		// if the segment size is zero, it obviously hasn't yet been allocated
			return;
		}

		// check that the segment exsists within the map
		auto & alloc_segments = mem_blocks[segment.block_id].alloc_segments;
		auto& iter = alloc_segments.find(segment.offset);

		if (iter != alloc_segments.end()) {

			// remove segment from the allocated list
			alloc_segments.erase(segment.offset);

			// and add to the free segments pool
			mem_blocks[segment.block_id].free_segments.insert(std::make_pair(segment.offset, segment.size));
		}
	}

	void VkMemoryManager::DestroyBlock(uint32_t id)
	{
		if (id < mem_blocks.size()) {

			// handle the vulkan side first
			vkDestroyBuffer(VulkanGlobal::vkCurrent.device, mem_blocks[id].block_buffer, nullptr);
			vkFreeMemory(VulkanGlobal::vkCurrent.device, mem_blocks[id].block_mem, nullptr);

			// and remove from the memory block pool
			mem_blocks.erase(mem_blocks.begin() + (id + 1));
		}
	}

	void VkMemoryManager::DestroyAllBlocks()
	{
		for (auto &block : mem_blocks) {

			DestroyBlock(block.block_id);
		}
	}

	// ================================================================ segmentInfo functions ====================================================================================================================

}