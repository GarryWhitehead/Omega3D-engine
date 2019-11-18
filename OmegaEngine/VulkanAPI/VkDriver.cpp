#include "VkDriver.h"

#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Managers/ProgramManager.h"
#include "VulkanAPI/VkTexture.h"

#include "Utility/logger.h"

#include <assert.h>

namespace VulkanAPI
{

VkDriver::VkDriver()
    : progManager(std::make_unique<ProgramManager>())
    , cbManager(std::make_unique<CommandBufferManager>())
{
}

VkDriver::~VkDriver()
{
	shutdown();
}

void VkDriver::init()
{
	// set up the memory allocator
	VmaAllocatorCreateInfo createInfo = {};
	createInfo.physicalDevice = context.physical;
	createInfo.device = context.device;
	vmaCreateAllocator(&createInfo, &vmaAlloc);

}

void VkDriver::shutdown()
{
	vmaDestroyAllocator(vmaAlloc);
}

// =========== functions for buffer/texture creation ================

VkBufferHandle VkDriver::addBuffer(const size_t size, VkBufferUsageFlags usage)
{
    Buffer buffer;
    buffer.prepare(vmaAlloc, static_cast<VkDeviceSize>(size), usage);
    buffers.emplace_back(buffer);
    return buffers.size() - 1;
}

void VkDriver::addVertexBuffer(const size_t size, void* data, std::vector<VertexBuffer::Attribute>& attributes)
{
    assert(data);
    VertexBuffer buffer;
    buffer.create(vmaAlloc, stagingPool, data, size);
    vertBuffers.emplace_back(buffer);
    return vertBuffers.size() - 1;
}

void VkDriver::addIndexBuffer(const size_t size, uint32_t* data)
{
    assert(data);
    IndexBuffer buffer;
    buffer.create(vmaAlloc, stagingPool, data, size);
    indexBuffers.emplace_back(buffer);
    return indexBuffers.size() - 1;
}

void VkDriver::add2DTexture(const vk::Format format, const uint32_t width, const uint32_t height, const uint8_t mipLevels, void* data)
{
    Texture tex;
    tex.create2dTex(format, width, height, mipLevels, usage);
    // map if data has passed through
    if (data)
    {
        tex.map(stagingPool, data);
    }
    textures.emplace_back(tex);
    return textures.size() - 1;
}

}    // namespace VulkanAPI
