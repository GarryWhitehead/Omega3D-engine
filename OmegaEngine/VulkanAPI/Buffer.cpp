#include "Buffer.h"

#include "VulkanAPI/CommandBuffer.h"

#include <cstring>

namespace VulkanAPI
{


void createBuffer(VmaAllocator& vmaAlloc, StagingPool& pool, VkBuffer& buffer, VmaAllocation& mem, void* data, VkDeviceSize dataSize)
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
    CmdBuffer cmdBuffer;
    
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = dataSize;
    vkCmdCopyBuffer(cmdBuffer.get(), stage.buffer, buffer, 1, &copyRegion);

    // clean-up
    pool.release(stage);
}

// ================== StagingPool =======================
void StagingPool::release(StageInfo& stage)
{
	freeStages.emplace_back(std::make_pair(stage.size, stage));
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
	freeStages.emplace_back(std::make_pair(reqSize, stage));
	return freeStages.back().second;
}

// ==================== Buffer ==========================

void Buffer::prepare(VmaAllocator& vmaAlloc, const vk::DeviceSize size, const VkBufferUsageFlags usage,
                     uint32_t memIndex)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
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
	vmaDestroyBuffer(vmaAlloc, buffer, mem);
}

// ================ Vertex buffer =======================
// Note: The following functions use VMA hence don't use the vulkan hpp C++ style vulkan bindings
// Hence, the code has been left in this verbose format - as it doesn't follow the format of the resdt of the codebase

void VertexBuffer::create(VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize dataSize, std::vector<Attribute>& attributes)
{
	assert(data);
    
    // create the buffer and copy the data
    createBuffer(vmaAlloc, pool, buffer, mem, data, dataSize);
    
    // create the attibutes for the vertex which will be added to the pipeline
    // calculate the offset for each location - the size of each location is store temporarily in the offset elemnt of the struct
    size_t nextOffset = 0;
    size_t currentOffset = 0;
    size_t totalSize = 0;
    size_t attributeCount = attributes.size();
    
    // finialise the attribute definitions except for the offsets
    uint8_t loc = 0;
    for (const Attribute& attr : attributes)
    {
        vertexAttrDescr.push_back({ loc++, 0, attr.format, atrr.stride });
    }
    
    // calculate the offset for each attribute based on the size of the last
    for (size_t i = 0; i < attributeCount; ++i)
    {
        nextOffset = attributes[i].offset;
        vertexAttrDescr[i].offset = currentOffset;
        currentOffset += nextOffset;
        totalSize += nextOffset;
    }

    // assuming just one binding at the moment - TODO: should also support instancing
    vk::VertexInputBindingDescription bindDescr(0, totalSize,
                                                 vk::VertexInputRate::eVertex);
    vertexBindDescr.push_back(bindDescr);
}

// ======================= IndexBuffer ================================

void IndexBuffer::create(VmaAllocator& vmaAlloc, StagingPool& pool, void* data, const VkDeviceSize dataSize)
{
	assert(data);
    
    // create the buffer and copy the data
	createBuffer(vmaAlloc, pool, buffer, mem, data, dataSize);
}

}    // namespace VulkanAPI
