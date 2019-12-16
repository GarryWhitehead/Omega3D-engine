#include "VkDriver.h"

#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkTexture.h"

#include "Utility/logger.h"

#include <assert.h>

namespace VulkanAPI
{

VkDriver::VkDriver()
    : progManager(std::make_unique<ProgramManager>())
    , cbManager(std::make_unique<CmdBufferManager>())
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

void VkDriver::addUbo(Util::String id, size_t size, VkBufferUsageFlags usage)
{
	// check if the buffer already exists with the same id. If so, check the size of the current
	// buffer against the size of the requested buffer. If the space is too small, destroy the existing buffer
	// and create a new one.
	auto iter = buffers.find({ id.c_str() });
	if (iter != buffers.end())
	{
		// check the size
		Buffer& buffer = iter->second;
		if (size < buffer.getSize())
		{
			// nothing else to do here as the buffer is of adequate size
			return;
		}
		deleteUbo(id);
	}

	Buffer buffer;
	buffer.prepare(vmaAlloc, static_cast<VkDeviceSize>(size), usage);
	buffers.emplace(id, buffer);

	// also update all descriptors that have this id - we haven't added data to the buffer yet but the descriptor sets are only
	// interested in the address and size of the buffer which is known now
	progManager->pushBufferDecsr(id, buffer);
}

void VkDriver::add2DTexture(Util::String id, vk::Format format, const uint32_t width, const uint32_t height,
                            const uint8_t mipLevels)
{
	// for textures, we expect the ids to be unique.
	auto iter = textures.find({ id.c_str() });
	assert(iter == textures.end());

	Texture tex;
	tex.create2dTex(format, width, height, mipLevels, usage);
	textures.emplace(id.c_str(), tex);

	// update the image descriptor set
	progManager->pushImageDecsr(id, tex);
}

VertexBuffer* VkDriver::addVertexBuffer(const size_t size, void* data)
{
	assert(data);
	VertexBuffer* buffer = new VertexBuffer;
	buffer->create(vmaAlloc, stagingPool, data, size);
	VkHash::ResourcePtrKey key = { static_cast<void*>(buffer) };
	vertBuffers.emplace(key, buffer);
	return buffer;
}

IndexBuffer* VkDriver::addIndexBuffer(const size_t size, uint32_t* data)
{
	assert(data);
	IndexBuffer* buffer = new IndexBuffer;
	buffer->create(vmaAlloc, stagingPool, data, size);
	VkHash::ResourcePtrKey key = { static_cast<void*>(buffer) };
	vertBuffers.emplace(key, buffer);
	return buffer;
}

// ========================= resource updates ===================================================

void VkDriver::update2DTexture(Util::String id, size_t size, void* data)
{
	auto iter = textures.find({ id.c_str() });
	assert(iter != textures.end());
	assert(data);

	iter->second.map(stagingPool, data);
}

void VkDriver::updateUbo(Util::String id, size_t size, void* data)
{
	auto iter = buffers.find({ id.c_str() });
	assert(iter != buffers.end());
	assert(data);

	iter->second.map(data, size);
}


// ============================ delete resources ==============================================


void VkDriver::deleteUbo(Util::String id)
{
	auto& iter = buffers.find({ id.c_str() });
	assert(iter != buffers.end());
	context.device.destroy(iter->second.get(), nullptr);
	buffers.erase({ id.c_str() });
}

void VkDriver::deleteVertexBuffer(VertexBuffer* buffer)
{
	auto iter = vertBuffers.find({ buffer });
	assert(iter != vertBuffers.end());
	context.device.destroy(iter->second->get(), nullptr);
	vertBuffers.erase({ buffer });
	delete buffer;
	buffer = nullptr;
}

void VkDriver::deleteIndexBuffer(IndexBuffer* buffer)
{
	auto iter = indexBuffers.find({ buffer });
	assert(iter != indexBuffers.end());
	context.device.destroy(iter->second->get(), nullptr);
	indexBuffers.erase({ buffer });
	delete buffer;
	buffer = nullptr;
}

// ======================== resource retrieval ===================================

Texture* VkDriver::getTexture2D(Util::String name)
{
	auto iter = textures.find({ name.c_str });
	if (iter == textures.end())
	{
		return nullptr;
	}
	return &iter->second;
}

Buffer* VkDriver::getBuffer(Util::String name)
{
	auto iter = buffers.find({ name.c_str });
	if (iter == buffers.end())
	{
		return nullptr;
	}
	return &iter->second;
}

// ============ begin/end frame functions ======================

void VkDriver::beginFrame()
{
	// TODO: need to reset some stuff here!

	// get the next image index which will be the framebuffer we draw to
	context.getDevice().acquireNextImageKHR(swapchain.get(), std::numeric_limits<uint64_t>::max(), swapchain.imageWait,
	                                        {}, &swapchain.imageIndex);
}

void VkDriver::endFrame()
{
	// submit all of the cmd buffers - it's supposedly a good idea to keep the number of cmd buffers to a min, as this can impact on performance(?)
	cbManager->submitAll();

	// finally, submit the final composaition to the present queue
	vk::PresentInfoKHR presentInfo{ 1, &swapchain.renderComplete, 1, &swapchain, &swapchain.imageIndex, nullptr };

	VK_CHECK_RESULT(context.getPresentQueue().presentKHR(&presentInfo));
}

}    // namespace VulkanAPI
