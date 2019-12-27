#pragma once

#include "VulkanAPI/Common.h"

#include <vector>

namespace VulkanAPI
{

/**
* @brief A simplisitic staging pool for CPU-only stages. Used when copying to GPU only mem
*/

class StagingPool
{
public:
    
	StagingPool(VmaAllocator& vmaAlloc);

	struct StageInfo
	{
		VkBuffer buffer;
		VkDeviceSize size;
		VmaAllocation mem;
		VmaAllocationInfo allocInfo;
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
    
    enum Usage
    {
        Dynamic,
        Static
    };
    
	Buffer() = default;

	void prepare(VmaAllocator& vmaAlloc, const vk::DeviceSize size, const VkBufferUsageFlags usage,
	             uint32_t memIndex = 0);

	void destroy();

	void map(void* data, size_t size);

	vk::Buffer get() const
	{
		return vk::Buffer(buffer);
	}

	size_t getSize() const
	{
		return static_cast<size_t>(size);
	}

private:
	VmaAllocationInfo allocInfo;
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

	vk::Buffer get()
	{
		return vk::Buffer(buffer);
	}

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

	void create(VmaAllocator& vmaAlloc, StagingPool& pool, uint32_t* data, const VkDeviceSize size);

	vk::Buffer get()
	{
		return vk::Buffer(buffer);
	}

private:
	VmaAllocation mem;
	VkDeviceSize size;
	VkBuffer buffer;
};

/**
 * @brief Creates a transient CPU staging buffer, copys that specified data to that, creates a GPU buffer and copies the
 * staging pool data to that.
 */
static void createGpuBufferAndCopy(VmaAllocator& vmaAlloc, StagingPool& pool, VkBuffer& buffer, VmaAllocation& mem,
                                   void* data, VkDeviceSize dataSize);

}    // namespace VulkanAPI
