#include "Queue.h"

namespace VulkanAPI
{
	Queue::Queue()
	{

	}

	Queue::Queue(vk::Queue q, vk::Device dev) :
		queue(q),
		device(dev)
	{
	}


	Queue::~Queue()
	{
	}

	void Queue::begin_frame()
	{
		assert(swap_chain);
		device.acquireNextImageKHR(swap_chain, std::numeric_limits<uint64_t>::max(), image_semaphore, {}, &image_index);
	}

	void Queue::submit_frame()
	{
		vk::PresentInfoKHR present_info(
			1, &present_semaphore,
			1, &swap_chain,
			&image_index,
			nullptr);

		VK_CHECK_RESULT(queue.presentKHR(&present_info));
		queue.waitIdle();
	}

	void Queue::submit_cmd_buffer(std::vector<vk::CommandBuffer>& cmd_buffers, std::vector<vk::Semaphore>& wait_semaphores, std::vector<vk::Semaphore>& signal_semaphores, vk::PipelineStageFlags* stage_flags)
	{
		assert(!cmd_buffers.empty() && !wait_semaphores.empty() && !signal_semaphores.empty() && stage_flags != nullptr);

		vk::SubmitInfo submit_info(
			static_cast<uint32_t>(wait_semaphores.size()), wait_semaphores.data(),
			stage_flags,
			static_cast<uint32_t>(cmd_buffers.size()), cmd_buffers.data(),
			static_cast<uint32_t>(signal_semaphores.size()), signal_semaphores.data());

		VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
		queue.waitIdle();
	}

	void Queue::submit_cmd_buffer(vk::CommandBuffer& cmd_buffer, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore, vk::PipelineStageFlags stage_flag)
	{
		vk::SubmitInfo submit_info(
			1, &wait_semaphore,
			&stage_flag,
			1, &cmd_buffer,
			1, &signal_semaphore);

		VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
		queue.waitIdle();
	}

	void Queue::submit_cmd_buffer(vk::CommandBuffer cmd_buffer)
	{
		vk::SubmitInfo submit_info(
			0, nullptr, nullptr,
			1, &cmd_buffer,
			0, nullptr);

		VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
		queue.waitIdle();
	}

}
