#include "VkDriver.h"

#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Managers/ProgramManager.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/SwapChain.h"

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

VertexBuffer* VkDriver::addVertexBuffer(const size_t size, void* data, std::vector<VertexBuffer::Attribute>& attributes)
{
    assert(data);
    VertexBuffer buffer;
    buffer.create(vmaAlloc, stagingPool, data, size);
	vertBuffers.emplace(data, buffer);
    return &vertBuffers[data];
}

IndexBuffer* VkDriver::addIndexBuffer(const size_t size, uint32_t* data)
{
    assert(data);
    IndexBuffer buffer;
    buffer.create(vmaAlloc, stagingPool, data, size);
	indexBuffers.emplace(data, buffer);
    return &indexBuffers[data];
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

// ============ begin/end frame functions ======================
void VkDriver::beginFrame()
{
	// TODO: need to reset some stuff here!

	// get the next image index which will be the framebuffer we draw to
	context.getDevice().acquireNextImageKHR(swapchain.get(), std::numeric_limits<uint64_t>::max(), swapchain.imageWait, {}, &swapchain.imageIndex);
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
