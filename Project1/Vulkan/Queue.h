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
		void begin_frame();
		void submit_frame();

		void submit_cmd_buffer(std::vector<vk::CommandBuffer>& cmd_buffer, std::vector<vk::Semaphore>& wait_semaphores, std::vector<vk::Semaphore>& signal_semaphores, vk::PipelineStageFlags* stage_flags);
		void submit_cmd_buffer(vk::CommandBuffer& cmd_buffer, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore, vk::PipelineStageFlags stage_flag);
		void submit_cmd_buffer(vk::CommandBuffer cmd_buffer);

		void create(vk::Queue q, vk::Device dev)
		{
			assert(q != VK_NULL_HANDLE);
			queue = q;
			device = dev;
		}

		void set_swapchain(vk::SwapchainKHR& sc)
		{
			assert(sc != VK_NULL_HANDLE);
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

		vk::Semaphore image_semaphore;
		vk::Semaphore present_semaphore;
	};

}

