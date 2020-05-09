/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "Buffer.h"

#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"

#include <cstring>

namespace VulkanAPI
{

void createGpuBufferAndCopy(
    VkDriver& driver,
    VmaAllocator& vmaAlloc,
    StagingPool& pool,
    VkBuffer& buffer,
    VmaAllocation& mem,
    void* data,
    VkDeviceSize dataSize,
    VkBufferUsageFlags usage)
{
    assert(data);
    
    // get a staging pool for hosting on the CPU side
    StagingPool::StageInfo stage = pool.getStage(dataSize);

    // copy data to staging area
    void* mapped;
    vmaMapMemory(vmaAlloc, stage.mem, &mapped);
    memcpy(mapped, data, dataSize);
    vmaUnmapMemory(vmaAlloc, stage.mem);
    vmaFlushAllocation(vmaAlloc, stage.mem, stage.allocInfo.offset, dataSize);

    // create GPU memory
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VMA_CHECK_RESULT(vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocInfo, &buffer, &mem, nullptr));

    // copy from the staging area to the allocated GPU memory
    auto& manager = driver.getCbManager();
    CmdBuffer* cmdBuffer = manager.getWorkCmdBuffer();

    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = dataSize;
    vkCmdCopyBuffer(cmdBuffer->get(), stage.buffer, buffer, 1, &copyRegion);

    // ensure that the copy finishes before the next frames draw call
    vk::BufferMemoryBarrier memBarrier {vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, stage.buffer, 0, VK_WHOLE_SIZE};
    cmdBuffer->get().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eVertexInput, vk::DependencyFlags(0), 0, nullptr, 1, &memBarrier, 0, nullptr);
    
    cmdBuffer->flush();

    // clean-up
    pool.release(stage);
}

// ================== StagingPool =======================
StagingPool::StagingPool(VmaAllocator& vmaAlloc) : vmaAlloc(vmaAlloc)
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
    assert(size > 0);

    StageInfo stage;
    stage.size = size;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.size = size;

    // cpu staging pool
    VmaAllocationCreateInfo createInfo = {};
    createInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    VMA_CHECK_RESULT(vmaCreateBuffer(
        vmaAlloc, &bufferInfo, &createInfo, &stage.buffer, &stage.mem, &stage.allocInfo));

    return stage;
}

StagingPool::StageInfo StagingPool::getStage(VkDeviceSize reqSize)
{
    // check for a free staging space that is equal of greater than the required size
    auto iter = std::lower_bound(
        freeStages.begin(),
        freeStages.end(),
        reqSize,
        [](const StageInfo& lhs, const VkDeviceSize rhs) { return lhs.size < rhs; });

    // if we have a free staging area, return that. Otherwise allocate a new stage
    if (iter != freeStages.end())
    {
        StageInfo stage = *iter;
        freeStages.erase(iter);
        return stage;
    }

    return create(reqSize);
}

// ==================== Buffer ==========================

void Buffer::prepare(
    VmaAllocator& vmaAlloc, const vk::DeviceSize buffSize, const VkBufferUsageFlags usage)
{
    vmaAllocator = &vmaAlloc;
    size = buffSize;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buffSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | usage;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    // allocCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VMA_CHECK_RESULT(
        vmaCreateBuffer(vmaAlloc, &bufferInfo, &allocCreateInfo, &buffer, &mem, &allocInfo));
}

void Buffer::map(void* data, size_t dataSize)
{
    assert(data);
    
    void* mapped;
    vmaMapMemory(*vmaAllocator, mem, &mapped);
    memcpy(mapped, data, dataSize);
    vmaUnmapMemory(*vmaAllocator, mem);
}

void Buffer::destroy()
{
    vmaDestroyBuffer(*vmaAllocator, buffer, mem);
}

vk::Buffer Buffer::get() const
{
    return vk::Buffer(buffer);
}

uint64_t Buffer::getSize() const
{
    return size;
}

uint64_t Buffer::getOffset() const
{
    return allocInfo.offset;
}

// ================ Vertex buffer =======================
// Note: The following functions use VMA hence don't use the vulkan hpp C++ style vulkan bindings
// Hence, the code has been left in this verbose format - as it doesn't follow the format of the
// rest of the codebase

void VertexBuffer::create(
    VkDriver& driver,
    VmaAllocator& vmaAlloc,
    StagingPool& pool,
    void* data,
    const VkDeviceSize dataSize)
{
    assert(data);

    // create the buffer and copy the data
    createGpuBufferAndCopy(
        driver, vmaAlloc, pool, buffer, mem, data, dataSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

// ======================= IndexBuffer ================================

void IndexBuffer::create(
    VkDriver& driver,
    VmaAllocator& vmaAlloc,
    StagingPool& pool,
    uint32_t* data,
    const VkDeviceSize dataSize)
{
    assert(data);

    // create the buffer and copy the data
    createGpuBufferAndCopy(
        driver, vmaAlloc, pool, buffer, mem, data, dataSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

} // namespace VulkanAPI
