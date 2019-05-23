#include "Interface.h"
#include "Vulkan/Device.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/SemaphoreManager.h"
#include "Vulkan/VkTextureManager.h"
#include "Vulkan/CommandBufferManager.h"

namespace VulkanAPI
{
	
	Interface::Interface(VulkanAPI::Device& dev, const uint32_t win_width, const uint32_t win_height, NewFrameMode mode)
	{
		// prepare all the core elements required for the vulkan API to render to screen
		// store locally all the handles we will need from the device
		device = dev.getDevice();
		gpu = dev.getPhysicalDevice();

		// prepare swap chain and attached image views - so we have something to render to
		swapchain_khr.create(device, gpu, dev.getSurface(), dev.getQueueIndex(Device::QueueType::Graphics),
			dev.getQueueIndex(Device::QueueType::Present), win_width, win_height);

		graphics_queue = dev.getQueue(Device::QueueType::Graphics);
		present_queue = dev.getQueue(Device::QueueType::Present);
		compute_queue = dev.getQueue(Device::QueueType::Compute);

		// init queues with swap-chain for ease of use later
		graphics_queue.set_swapchain(swapchain_khr.get());
		present_queue.set_swapchain(swapchain_khr.get());
		compute_queue.set_swapchain(swapchain_khr.get());

		// establish vulkan managers
		bufferManager = std::make_unique<BufferManager>(device, gpu, graphics_queue);
		textureManager = std::make_unique<VkTextureManager>(device, gpu, graphics_queue);
		cmdBufferManager = std::make_unique<CommandBufferManager>(device, gpu, graphics_queue, present_queue, swapchain_khr, mode);
	}


	Interface::~Interface()
	{
	}

}
