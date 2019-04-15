#include "Interface.h"
#include "Vulkan/Device.h"
#include "Vulkan/MemoryAllocator.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/SemaphoreManager.h"
#include "Vulkan/VkTextureManager.h"

namespace VulkanAPI
{
	
	Interface::Interface(VulkanAPI::Device dev, uint32_t win_width, uint32_t win_height)
	{
		// prepare all the core elements required for the vulkan API to render to screen
		// store locally all the handles we will need from the device
		device = dev.getDevice();
		gpu = dev.getPhysicalDevice();
		graphics_queue = dev.getQueue(Device::QueueType::Graphics);
		present_queue = dev.getQueue(Device::QueueType::Present);
		compute_queue = dev.getQueue(Device::QueueType::Compute);

		// prepare swap chain and attached image views - so we have something to render too
		swapchain_khr.create(dev, win_width, win_height);

		// init queues with swap-chain for ease of use later
		graphics_queue.set_swapchain(swapchain_khr.get());
		present_queue.set_swapchain(swapchain_khr.get());
		compute_queue.set_swapchain(swapchain_khr.get());

		// establish vulkan managers
		buffer_manager = std::make_unique<BufferManager>(device, gpu, graphics_queue.get());
		semaphore_manager = std::make_unique<SemaphoreManager>(device);
		texture_manager = std::make_unique<VkTextureManager>(device, gpu, graphics_queue);
	}


	Interface::~Interface()
	{
	}

}
