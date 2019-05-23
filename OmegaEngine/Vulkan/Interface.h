#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/Swapchain.h"
#include "Vulkan/Queue.h"

#include <string>
#include <memory>

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

		Interface(VulkanAPI::Device& device, const uint32_t win_width, const uint32_t win_height, NewFrameMode mode);
		~Interface();

		vk::Device& getDevice()
		{
			return device;
		}

		vk::PhysicalDevice& get_gpu()
		{
			return gpu;
		}

		Queue& getGraphicsQueue()
		{
			return graphics_queue;
		}

		Queue& get_present_queue()
		{
			return present_queue;
		}

		Swapchain& getSwapchain()
		{
			return swapchain_khr;
		}

		std::unique_ptr<BufferManager>& getBufferManager()
		{
			return bufferManager;
		}

		std::unique_ptr<VkTextureManager>& gettextureManager()
		{
			return textureManager;
		}

		std::unique_ptr<CommandBufferManager>& getCmdBufferManager()
		{
			return cmdBufferManager;
		}

	private:

		vk::Device device;
		vk::PhysicalDevice gpu;

		// queues associated with this device
		Queue graphics_queue;
		Queue present_queue;
		Queue compute_queue;

		// the swap-chain
		VulkanAPI::Swapchain swapchain_khr;

		// managers
		std::unique_ptr<BufferManager> bufferManager;
		std::unique_ptr<VkTextureManager> textureManager;
		std::unique_ptr<CommandBufferManager> cmdBufferManager;
	};

}

