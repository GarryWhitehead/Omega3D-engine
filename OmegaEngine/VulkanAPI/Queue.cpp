#include "Queue.h"

namespace VulkanAPI
{
Queue::Queue()
{
}

Queue::Queue(vk::Queue q, vk::Device dev)
    : queue(q)
    , device(dev)
{
}

Queue::~Queue()
{
}

void Queue::submitCmdBuffer(std::vector<vk::CommandBuffer> &cmdBuffers,
                            std::vector<vk::Semaphore> &waitSemaphores,
                            std::vector<vk::Semaphore> &signalSemaphores,
                            vk::PipelineStageFlags *stage_flags)
{
	assert(!cmdBuffers.empty() && !waitSemaphores.empty() && !signalSemaphores.empty() &&
	       stage_flags != nullptr);

	vk::PipelineStageFlags default_flag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info(static_cast<uint32_t>(waitSemaphores.size()), waitSemaphores.data(),
	                           stage_flags == nullptr ? &default_flag : stage_flags,
	                           static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data(),
	                           static_cast<uint32_t>(signalSemaphores.size()),
	                           signalSemaphores.data());

	VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
	queue.waitIdle();
}

void Queue::submitCmdBuffer(vk::CommandBuffer &cmdBuffer, vk::Semaphore &waitSemaphore,
                            vk::Semaphore &signalSemaphore, vk::Fence &fence)
{
	vk::PipelineStageFlags stage_flag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submit_info(1, &waitSemaphore, &stage_flag, 1, &cmdBuffer, 1, &signalSemaphore);

	VK_CHECK_RESULT(queue.submit(1, &submit_info, fence));
	queue.waitIdle();
}

void Queue::flushCmdBuffer(vk::CommandBuffer cmdBuffer)
{
	vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr);

	VK_CHECK_RESULT(queue.submit(1, &submit_info, {}));
	queue.waitIdle();
}

} // namespace VulkanAPI
