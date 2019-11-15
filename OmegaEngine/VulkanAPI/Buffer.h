#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

/**
* @brief A simplisitic staging pool for CPU-only stages. Used when copying to GPU only mem
*/

class StagingPool
{
public:
	
	struct StageInfo
	{
		VkBuffer buffer;
		VkDeviceSize size;
		VmaAllocation mem;
	};

	StageInfo create(const VkDeviceSize size);

	StageInfo& getStage(const VkDeviceSize reqSize); 

	void release(StageInfo& stage);

private:

	// keep a refernce to the memory allocator here
	VmaAllocator& vmaAlloc;

	// a list of free stages and their size
	std::vector<std::pair<VkDeviceSize, StageInfo>> freeStages;
};


/** @brief A wrapper around a VkBuffer allowing easier mem allocation using VMA
* This is for dynamic mem type allocation, i.e. uniform buffers, etc.
*/
class Buffer
{
public:
	Buffer() = default;

	void prepare(VmaAllocator& vmaAlloc, const vk::DeviceSize size, const VkBufferUsageFlags usage,
	             uint32_t memIndex = 0);

	void destroy();

	void map(void* data, size_t size);

private:
	VmaAllocation mem;
	VkDeviceSize size;
	VkBuffer buffer;
};

/**
* @brief A special buffer type for storing vertex data - mapped to the CPU stage, and copied to the GPU side
* Note: For static data.
*/
class VertexBuffer
{
public:
	VertexBuffer() = default;

	void create(VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize size);

private:
	VmaAllocation mem;
	VkDeviceSize size;
	VkBuffer buffer;
};

/**
* @brief A special buffer type for storing indices data - mapped to the CPU stage, and copied to the GPU side
* Note: Indices data must be 32-bit integer format. For static data.
*/
class IndexBuffer
{
public:
	IndexBuffer() = default;

	void create(VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize size);

private:
	VmaAllocation mem;
	VkDeviceSize size;
	VkBuffer buffer;
};

}    // namespace VulkanAPI