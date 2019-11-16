#include "Buffer.h"

namespace VulkanAPI
{

// ================== StagingPool =======================
void StagingPool::release(StageInfo& stage)
{
	freeStages.emplace();
}

StagingPool::StageInfo StagingPool::create(const VkDeviceSize size)
{
	StageInfo stage;
	stage.size = size;

	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// cpu staging pool
	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
	allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &stage.buffer, &stage.mem, nullptr));

	return stage;
}

StagingPool::StageInfo& StagingPool::getStage(const VkDeviceSize reqSize)
{
	// check for a free staging space that is equal of greater than the required size
	auto iter =
	    std::lower_bound(freeStages.begin(), freeStages.end(),
	                     [](const std::pair<VkDeviceSize, StageInfo>& lhs,
	                        const std::pair<VkDeviceSize, StageInfo>& rhs) -> bool { return lhs.first < rhs.first; });

	// if we have a free staging area, return that. Otherwise allocate a new stage
	if (iter != freeStages.end())
	{
		return iter->second;
	}

	StageInfo stage = create(reqSize);
	freeStages.emplace(reqSize, stage);
	return freeStages.back();
}

// ==================== Buffer ==========================

void Buffer::prepare(VmaAllocator& vmaAlloc, const vk::DeviceSize size, const VkBufferUsageFlags usage,
                     uint32_t memIndex)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocInfo.memoryTypeBits = memIndex;

	VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &buffer, &mem, nullptr));
}

void Buffer::map(void* data, size_t size)
{
	assert(data);
	memcpy(mem.pMappedData, data, size);
}

void Buffer::destroy()
{
	vmaDestroyBuffer(vmaAlloc, buffer, mem);
}

// ================ Vertex buffer =======================
// Note: The following functions use VMA hence don't use the vulkan hpp C++ style vulkan bindings
// Hence, the code has been left in this verbose format - as it doesn't follow the format of the resdt of the codebase

void VertexBuffer::create(VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize size)
{
	assert(data);
	// get a staging pool for hosting on the CPU side
	StagingPool::StageInfo stage = pool.getStage(size);

	// copy data to staging area
	memcpy(stage.mem.pMappedData, data, size);

	// create GPU memory
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &buffer, &mem, nullptr));

	// copy from the staging area to the allocated GPU memory
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(cmdBuffer, stage.buffer, buffer, 1, &copyRegion);

	// clean-up
	pool.release(stage);
}

// ======================= IndexBuffer ================================

void IndexBuffer::create(VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize size)
{
	assert(data);
	// get a staging pool for hosting on the CPU side
	StagingPool::StageInfo stage = pool.getStage(size);

	// copy data to staging area
	memcpy(stage.mem.pMappedData, data, size);

	// create GPU memory
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &buffer, &mem, nullptr));

	// copy from the staging area to the allocated GPU memory
	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(cmdBuffer, stage.buffer, buffer, 1, &copyRegion);

	// clean-up
	pool.release(stage);
}

}    // namespace VulkanAPI
