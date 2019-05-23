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

	void Queue::submit_cmdBuffer(std::vector<vk::CommandBuffer>& cmdBuffers, std::vector<vk::Semaphore>& wait_semaphores, std::vector<vk::Semaphore>& signal_semaphores, vk::PipelineStageFlags* stage_flags)
	{
		assert(!cmdBuffers.empty() && !wait_semaphores.empty() && !signal_semaphores.empty() && stage_flags != nullptr);

		vk::PipelineStageFlags default_flag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		vk::SubmitInfo submit_info(
			static_cast<uint32_t>(wait_semaphores.size()), wait_semaphores.data(),
			stage_flags == nullptr ? &default_flag : stage_flags,
			static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data(),
			static_cast<uint32_t>(signal_semaphores.size()), signal_semaphores.data());

		VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
		queue.waitIdle();
	}

	void Queue::submit_cmdBuffer(vk::CommandBuffer& cmdBuffer, vk::Semaphore& wait_semaphore, vk::Semaphore& signal_semaphore, vk::Fence& fence)
	{
		vk::PipelineStageFlags stage_flag = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		
		vk::SubmitInfo submit_info(
			1, &wait_semaphore,
			&stage_flag,
			1, &cmdBuffer,
			1, &signal_semaphore);

		VK_CHECK_RESULT(queue.submit(1, &submit_info, fence));
		queue.waitIdle();
	}

	void Queue::flush_cmdBuffer(vk::CommandBuffer cmdBuffer)
	{
		vk::SubmitInfo submit_info(
			0, nullptr, nullptr,
			1, &cmdBuffer,
			0, nullptr);

		VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
		queue.waitIdle();
	}

}
