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

#pragma once

#include "VulkanAPI/Common.h"

#include <vector>

namespace VulkanAPI
{

// forward declerations
struct VkContext;
class VkDriver;

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

    StageInfo getStage(VkDeviceSize reqSize);

    void release(StageInfo& stage);

private:
    // keep a refernce to the memory allocator here
    VmaAllocator& vmaAlloc;

    // a list of free stages and their size
    std::vector<StageInfo> freeStages;
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

    void prepare(VmaAllocator& vmaAlloc, const vk::DeviceSize size, const VkBufferUsageFlags usage);

    void destroy();

    void map(void* data, size_t size);

    vk::Buffer get() const;
    uint64_t getSize() const;
    uint64_t getOffset() const;

private:
    VmaAllocator* vmaAllocator = nullptr;

    VmaAllocationInfo allocInfo;
    VmaAllocation mem;
    VkDeviceSize size;
    VkBuffer buffer;
};

/**
 * @brief A special buffer type for storing vertex data - mapped to the CPU stage, and copied to the
 * GPU side Note: For static data.
 */
class VertexBuffer
{
public:
    VertexBuffer() = default;

    void create(
        VkDriver& driver,
        VmaAllocator& vmaAlloc,
        StagingPool& pool,
        void* data,
        const VkDeviceSize size);

    vk::Buffer get()
    {
        return vk::Buffer(buffer);
    }

    uint32_t getOffset() const;

private:

    VmaAllocationInfo allocInfo;
    VmaAllocation mem;
    VkDeviceSize size;
    VkBuffer buffer;
};

/**
 * @brief A special buffer type for storing indices data - mapped to the CPU stage, and copied to
 * the GPU side Note: Indices data must be 32-bit integer format. For static data.
 */
class IndexBuffer
{
public:
    IndexBuffer() = default;

    void create(
        VkDriver& driver,
        VmaAllocator& vmaAlloc,
        StagingPool& pool,
        uint32_t* data,
        const VkDeviceSize size);

    vk::Buffer get()
    {
        return vk::Buffer(buffer);
    }

     uint32_t getOffset() const;

private:

    VmaAllocationInfo allocInfo;
    VmaAllocation mem;
    VkDeviceSize size;
    VkBuffer buffer;
};

/**
 * @brief Creates a transient CPU staging buffer, copys that specified data to that, creates a GPU
 * buffer and copies the staging pool data to that.
 */
inline void createGpuBufferAndCopy(
    VkDriver& driver,
    VmaAllocator& vmaAlloc,
    StagingPool& pool,
    VkBuffer& buffer,
    VmaAllocation& mem,
    VmaAllocationInfo& allocInfo,
    void* data,
    VkDeviceSize dataSize);

} // namespace VulkanAPI
