#pragma once
#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

class Queue
{

public:
	Queue();
	Queue(vk::Queue q, vk::Device dev);
	~Queue();

	void submitCmdBuffer(std::vector<vk::CommandBuffer> &cmdBuffer,
	                     std::vector<vk::Semaphore> &waitSemaphores,
	                     std::vector<vk::Semaphore> &signalSemaphores,
	                     vk::PipelineStageFlags *stageFlags = nullptr);
	void submitCmdBuffer(vk::CommandBuffer &cmdBuffer, vk::Semaphore &waitSemaphore,
	                     vk::Semaphore &signalSemaphore, vk::Fence &fence);
	void flushCmdBuffer(vk::CommandBuffer cmdBuffer);

	void create(vk::Queue &q, vk::Device &dev, const uint32_t queueIndex)
	{
		assert(q);
		queue = q;
		device = dev;
		queueFamilyIndex = queueIndex;
	}

	void setSwapchain(vk::SwapchainKHR &sc)
	{
		assert(sc);
		swapchain = sc;
	}

	vk::Queue &get()
	{
		return queue;
	}

	uint32_t getIndex() const
	{
		return queueFamilyIndex;
	}

private:
	vk::Device device;
	vk::Queue queue;
	vk::SwapchainKHR swapchain;

	uint32_t queueFamilyIndex = 0;
};

} // namespace VulkanAPI
