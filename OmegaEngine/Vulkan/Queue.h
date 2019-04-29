#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{

	class Queue
	{

	public:

		Queue();
		Queue(vk::Queue q, vk::Device dev);
		~Queue();

		void submit_cmd_buffer(std::vector<vk::CommandBuffer>& cmd_buffer, std::vector<vk::Semaphore>& wait_semaphores, std::vector<vk::Semaphore>& signal_semaphores, vk::PipelineStageFlags* stage_flags = nullptr);
		void submit_cmd_buffer(vk::CommandBuffer& cmd_buffer, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore, vk::Fence& fence);
		void flush_cmd_buffer(vk::CommandBuffer cmd_buffer);

		void create(vk::Queue& q, vk::Device& dev, const uint32_t queue_index)
		{
			assert(q);
			queue = q;
			device = dev;
			queue_family_index = queue_index;
		}

		void set_swapchain(vk::SwapchainKHR& sc)
		{
			assert(sc);
			swap_chain = sc;
		}

		vk::Queue& get()
		{
			return queue;
		}

		uint32_t get_index() const
		{
			return queue_family_index;
		}

	private:

		vk::Device device;
		vk::Queue queue;
		vk::SwapchainKHR swap_chain;

		uint32_t queue_family_index = 0;
	};

}

