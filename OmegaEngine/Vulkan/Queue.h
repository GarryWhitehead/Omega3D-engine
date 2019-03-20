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

		// frame submit - must be done with the presentation queue
		void begin_frame(vk::Semaphore& image_semaphore);
		void submit_frame(vk::Semaphore& present_semaphore);

		void submit_cmd_buffer(std::vector<vk::CommandBuffer>& cmd_buffer, std::vector<vk::Semaphore>& wait_semaphores, std::vector<vk::Semaphore>& signal_semaphores, vk::PipelineStageFlags* stage_flags);
		void submit_cmd_buffer(vk::CommandBuffer& cmd_buffer, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore, vk::PipelineStageFlags stage_flag);
		void flush_cmd_buffer(vk::CommandBuffer cmd_buffer);

		void create(vk::Queue q, vk::Device dev)
		{
			assert(q);
			queue = q;
			device = dev;
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

	private:

		vk::Device device;
		vk::Queue queue;
		vk::SwapchainKHR swap_chain;

		uint32_t image_index = 0;
	};

}

