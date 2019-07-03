#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Queue.h"
#include "VulkanAPI/Swapchain.h"

#include <memory>
#include <string>

namespace VulkanAPI
{
// forward declerations
class BufferManager;
class VkTextureManager;
class CommandBufferManager;
class Device;
enum class NewFrameMode;

class Interface
{

public:
	Interface(VulkanAPI::Device &device, const uint32_t windowWidth, const uint32_t winHeight,
	          NewFrameMode mode);
	~Interface();

	vk::Device &getDevice()
	{
		return device;
	}

	vk::PhysicalDevice &getGpu()
	{
		return gpu;
	}

	Queue &getGraphicsQueue()
	{
		return graphicsQueue;
	}

	Queue &getPresentionQueue()
	{
		return presentionQueue;
	}

	Swapchain &getSwapchain()
	{
		return swapchainKhr;
	}

	std::unique_ptr<BufferManager> &getBufferManager()
	{
		return bufferManager;
	}

	std::unique_ptr<VkTextureManager> &gettextureManager()
	{
		return textureManager;
	}

	std::unique_ptr<CommandBufferManager> &getCmdBufferManager()
	{
		return cmdBufferManager;
	}

private:
	vk::Device device;
	vk::PhysicalDevice gpu;

	// queues associated with this device
	Queue graphicsQueue;
	Queue presentionQueue;
	Queue computeQueue;

	// the swap-chain
	VulkanAPI::Swapchain swapchainKhr;

	// managers
	std::unique_ptr<BufferManager> bufferManager;
	std::unique_ptr<VkTextureManager> textureManager;
	std::unique_ptr<CommandBufferManager> cmdBufferManager;
};

} // namespace VulkanAPI
