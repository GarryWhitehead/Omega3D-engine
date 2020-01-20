#include "Buffer.h"

#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"

#include <cstring>

namespace VulkanAPI
{

void createGpuBufferAndCopy(VkDriver& driver, VmaAllocator& vmaAlloc, StagingPool& pool, VkBuffer& buffer, VmaAllocation& mem, void* data, VkDeviceSize dataSize)
{
    // get a staging pool for hosting on the CPU side
    StagingPool::StageInfo stage = pool.getStage(dataSize);

    // copy data to staging area
    memcpy(stage.allocInfo.pMappedData, data, dataSize);

    // create GPU memory
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &buffer, &mem, nullptr));

    // copy from the staging area to the allocated GPU memory
    auto& manager = driver.getCbManager();
    std::unique_ptr<CmdBuffer> cmdBuffer = manager.getSingleUseCb();
    
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = dataSize;
    vkCmdCopyBuffer(cmdBuffer->get(), stage.buffer, buffer, 1, &copyRegion);
    
    cmdBuffer->flush();
    
    // clean-up
    pool.release(stage);
    manager.releaseSingleUseCb(std::move(cmdBuffer));
}

// ================== StagingPool =======================
StagingPool::StagingPool(VmaAllocator& vmaAlloc) :
    vmaAlloc(vmaAlloc)
{
}

void StagingPool::release(StageInfo& stage)
{
    assert(stage.size > 0);
    assert(stage.buffer);
    freeStages.emplace_back(stage);
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
	VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &stage.buffer, &stage.mem, &stage.allocInfo));

	return stage;
}

StagingPool::StageInfo& StagingPool::getStage(VkDeviceSize reqSize)
{
	// check for a free staging space that is equal of greater than the required size
	auto iter =
	    std::lower_bound(freeStages.begin(), freeStages.end(), reqSize,
	                     [](const StageInfo& lhs, const VkDeviceSize rhs) { return lhs.size < rhs; });

	// if we have a free staging area, return that. Otherwise allocate a new stage
	if (iter != freeStages.end())
	{
		return *iter;
	}

	StageInfo stage = create(reqSize);
	freeStages.emplace_back(stage);
	return freeStages.back();
}

// ==================== Buffer ==========================

void Buffer::prepare(VmaAllocator& vmaAlloc, const vk::DeviceSize buffSize, const VkBufferUsageFlags usage,
                     uint32_t memIndex)
{
    vmaAllocator = &vmaAlloc;
    
    VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = buffSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	allocCreateInfo.memoryTypeBits = memIndex;

	VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocCreateInfo, &buffer, &mem, &allocInfo));
}

void Buffer::map(void* data, size_t dataSize)
{
	assert(data);
    memcpy(allocInfo.pMappedData, data, dataSize);
}

void Buffer::destroy()
{
	vmaDestroyBuffer(*vmaAllocator, buffer, mem);
}

// ================ Vertex buffer =======================
// Note: The following functions use VMA hence don't use the vulkan hpp C++ style vulkan bindings
// Hence, the code has been left in this verbose format - as it doesn't follow the format of the rest of the codebase

void VertexBuffer::create(VkDriver& driver, VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize dataSize)
{
	assert(data);
    
    // create the buffer and copy the data
    createGpuBufferAndCopy(driver, vmaAlloc, pool, buffer, mem, data, dataSize);
    
}

// ======================= IndexBuffer ================================

void IndexBuffer::create(VkDriver& driver, VmaAllocator& vmaAlloc, StagingPool& pool, uint32_t* data, const VkDeviceSize dataSize)
{
	assert(data);
    
    // create the buffer and copy the data
	createGpuBufferAndCopy(driver, vmaAlloc, pool, buffer, mem, data, dataSize);
}

}    // namespace VulkanAPI