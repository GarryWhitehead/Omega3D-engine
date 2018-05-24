#pragma once
#include <vector>
#include <unordered_map>
#include "VulkanCore/Vulkan_utility.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class VulkanEngine;

enum class MemoryType
{
	VK_BLOCK_TYPE_HOST,
	VK_BLOCK_TYPE_LOCAL
};

enum class MemoryUsage
{
	VK_UNIFORM_BUFFER = 1 << 0,		
	VK_STORAGE_BUFFER = 1 << 1,
	VK_VERTEX_BUFFER = 1 << 2,
	VK_INDEX_BUFFER = 1 << 3,
	VK_BUFFER_STATIC = 1 << 4,		// refers to device-local data were the data isn't updated 
	VK_BUFFER_DYNAMIC = 1 << 5	
};

inline bool operator& (MemoryUsage a, MemoryUsage b)
{
	return static_cast<int>(a) & static_cast<int>(b);
}

inline MemoryUsage operator| (MemoryUsage a, MemoryUsage b)
{
	return static_cast<MemoryUsage>(static_cast<int>(a) | static_cast<int>(b));
}

class VkMemoryManager
{

public:
	
	// default block size - can be overriden by user
	static const uint32_t ALLOC_BLOCK_SIZE_LOCAL = 2.56e+8;			// each default device local block is 256mb
	static const uint32_t ALLOC_BLOCK_SIZE_HOST = 6.4e+7;			// host block deaults to 64mb

	struct SegmentInfo
	{
		SegmentInfo() : data(nullptr) {}
		SegmentInfo(uint32_t id, uint32_t os, uint32_t sz) :
			block_id(id),
			offset(os),
			size(sz),
			data(nullptr)
		{}

		int32_t block_id;			// index into memory block container
		uint32_t offset;
		uint32_t size;
		void *data;			
	};

	struct BlockInfo
	{
		int32_t block_id;

		MemoryType type;
		uint32_t total_size;

		std::unordered_map<uint32_t, uint32_t> alloc_segments;
		std::unordered_map<uint32_t, uint32_t> free_segments;		// fisrt = offset - second = size

		// vulkan info
		VkDeviceMemory block_mem;
		VkBuffer block_buffer;
	};

	VkMemoryManager(VulkanEngine *engine);
	~VkMemoryManager();

	// no copy assignment allowed
	VkMemoryManager(const VkMemoryManager&) = delete;
	VkMemoryManager& operator=(const VkMemoryManager&) = delete;

	// Block and segment allocation functions
	uint32_t AllocateBlock(MemoryType type, uint32_t size = 0);
	uint32_t AllocateBlock(MemoryUsage usage);
	SegmentInfo AllocateSegment(MemoryUsage usage, uint32_t size);
	bool CreateBuffer(const uint32_t size, const VkBufferUsageFlags flags, const VkMemoryPropertyFlags props, VkDeviceMemory& memory, VkBuffer& buffer);
	uint32_t FindFreeSegment(uint32_t block_id, uint32_t size);

	// helper functions
	uint32_t FindMemoryType(const uint32_t type, const VkMemoryPropertyFlags flags);
	uint32_t FindBlockType(MemoryUsage usage);
	void DefragBlockMemory(const uint32_t id);
	VkBuffer& blockBuffer(const uint32_t id);

	// memory mapping functions
	template <typename T>
	void MapDataToSegment(SegmentInfo &segment, std::vector<T> data);

	template <typename T>
	void MapData(void *mapped_data, VkDeviceMemory memory, const uint32_t offset, const uint32_t segment_size, std::vector<T>& data);

private:

	// omega engine specific pointer to vulkan engine
	VulkanEngine * p_vkEngine;

	std::vector<BlockInfo> mem_blocks;
};

template <typename T>
void VkMemoryManager::MapDataToSegment(SegmentInfo &segment, std::vector<T> data)
{
	assert(segment.block_id < mem_blocks.size());
	BlockInfo block = mem_blocks[segment.block_id];

	// How we map the data depends on the memory type; 
	// For host-visible memory, the memory is mapped and the data is just mem-copied across
	// With device local, we must first create a buffer on the host, map the data to that and then copyCmd it across to local memory

	if (block.type == MemoryType::VK_BLOCK_TYPE_HOST) {

		MapData(segment.data, block.block_mem, segment.offset, segment.size, data);
	}
	else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL) {

		// start by creating host-visible buffers
		VkBuffer temp_buffer;
		VkDeviceMemory temp_memory;
		CreateBuffer(segment.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, temp_memory, temp_buffer);

		// map data to temporary buffer
		MapData(segment.data, temp_memory, 0, segment.size, data);

		// create cmd buffer for copy and transfer to device local memory
		VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

		vkUtility->CopyBuffer(temp_buffer, block.block_buffer, segment.size, p_vkEngine->GetCmdPool(), 0, segment.offset);
		delete vkUtility;

		// clear up and we are done
		vkDestroyBuffer(p_vkEngine->GetDevice(), temp_buffer, nullptr);
		vkFreeMemory(p_vkEngine->GetDevice(), temp_memory, nullptr);
	}
}

template <typename T>
void VkMemoryManager::MapData(void *mapped_data, VkDeviceMemory memory, const uint32_t offset, const uint32_t segment_size, std::vector<T>& data)
{
	uint32_t data_size = sizeof(T) * data.size();
	assert(data_size <= segment_size);

	if (mapped_data == nullptr) {
		VK_CHECK_RESULT(vkMapMemory(p_vkEngine->GetDevice(), memory, offset, segment_size, 0, &mapped_data));
		memcpy(mapped_data, data.data(), data_size);
		vkUnmapMemory(p_vkEngine->GetDevice(), memory);
	}
	else {
		memcpy(mapped_data, data.data(), data_size);
	}
}
