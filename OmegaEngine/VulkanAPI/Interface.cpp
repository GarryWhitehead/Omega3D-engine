#include "Interface.h"
#include "VulkanAPI/BufferManager.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/Device.h"
#include "VulkanAPI/MemoryAllocator.h"
#include "VulkanAPI/SemaphoreManager.h"
#include "VulkanAPI/VkTextureManager.h"

namespace VulkanAPI
{

Interface::Interface(VulkanAPI::Device &dev, const uint32_t windowWidth, const uint32_t winHeight,
                     NewFrameMode mode)
{
	// prepare all the core elements required for the vulkan API to render to screen
	// store locally all the handles we will need from the device
	device = dev.getDevice();
	gpu = dev.getPhysicalDevice();

	// prepare swap chain and attached image views - so we have something to render to
	swapchainKhr.create(device, gpu, dev.getSurface(),
	                    dev.getQueueIndex(Device::QueueType::Graphics),
	                    dev.getQueueIndex(Device::QueueType::Present), windowWidth, winHeight);

	graphicsQueue = dev.getQueue(Device::QueueType::Graphics);
	presentionQueue = dev.getQueue(Device::QueueType::Present);
	computeQueue = dev.getQueue(Device::QueueType::Compute);

	// init queues with swap-chain for ease of use later
	graphicsQueue.setSwapchain(swapchainKhr.get());
	presentionQueue.setSwapchain(swapchainKhr.get());
	computeQueue.setSwapchain(swapchainKhr.get());

	// establish vulkan managers
	bufferManager = std::make_unique<BufferManager>(device, gpu, graphicsQueue);
	textureManager = std::make_unique<VkTextureManager>(device, gpu, graphicsQueue);
	cmdBufferManager = std::make_unique<CommandBufferManager>(device, gpu, graphicsQueue,
	                                                          presentionQueue, swapchainKhr, mode);
}

Interface::~Interface()
{
}

} // namespace VulkanAPI
