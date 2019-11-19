#include "VkDriver.h"

#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Managers/ProgramManager.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/VkContext.h"

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

void VkDriver::init(const char** instanceExt, uint32_t count)
{
	// prepare the vulkan backend
	// create a new vulkan instance
	context.createInstance(instanceExt, count);

	// prepare the physical and abstract device including queues
	context.prepareDevice();

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

void VkDriver::addStaticBuffer(const size_t size, VkBufferUsageFlags usage)
{
    Buffer buffer;
    buffer.prepare(vmaAlloc, static_cast<VkDeviceSize>(size), usage);
	buffers.emplace(data, buffer);
}

void VkDriver::addVertexBuffer(const size_t size, void* data, std::vector<VertexBuffer::Attribute>& attributes)
{
    assert(data);
    VertexBuffer buffer;
    buffer.create(vmaAlloc, stagingPool, data, size);
	vertBuffers.emplace(data, buffer);
}

void VkDriver::addIndexBuffer(const size_t size, uint32_t* data)
{
    assert(data);
    IndexBuffer buffer;
    buffer.create(vmaAlloc, stagingPool, data, size);
	indexBuffers.emplace(data, buffer);
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
    textures.emplace(data, tex);
}

}    // namespace VulkanAPI
