#include "Interface.h"
#include "Vulkan/Device.h"
#include "Vulkan/MemoryAllocator.h"

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
	
		// now we have init the current vulkan device, init all managers that will be used
		mem_allocator = std::make_unique<MemoryAllocator>(device, gpu);
	}


	Interface::~Interface()
	{
	}

}
