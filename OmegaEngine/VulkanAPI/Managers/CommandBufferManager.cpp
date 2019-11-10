#include "CommandBufferManager.h"

#include "VulkanAPI/Managers/SemaphoreManager.h"
#include "VulkanAPI/Renderpass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

CmdBufferManager::CmdBufferManager() : 
	context(context)
{
}

CmdBufferManager::~CmdBufferManager()
{
}

CmdBufferHandle CmdBufferManager::newInstance()
{
	CommandBufferInfo buffer_info;

	// create a fence which will be used to sync things
	vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
	VK_CHECK_RESULT(context->getDevice().createFence(&fence_info, nullptr, &buffer_info.fence));
	VK_CHECK_RESULT(context->getDevice().resetFences(1, &buffer_info.fence));

	// and the semaphore used to sync between queues
	buffer_info.semaphore = semaphoreManager->getSemaphore();

	cmdBuffers.emplace_back(std::move(buffer_info));

	return cmdBuffers.size() - 1;
}

std::unique_ptr<CommandBuffer>& CmdBufferManager::getCmdBuffer(CmdBufferHandle handle)
{
	assert(handle < cmdBuffers.size());
	return cmdBuffers[handle].cmdBuffer;
}

std::unique_ptr<VulkanAPI::CommandBuffer>& CmdBufferManager::beginNewFame(CmdBufferHandle handle)
{

	// if it's static then only create a new instance if it's null
	if (mode == NewFrameMode::Static)
	{
		if (cmdBuffers[handle].cmdBuffer == nullptr)
		{
			cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(dev, CommandBuffer::UsageType::Multi);
			cmdBuffers[handle].cmdBuffer->createPrimary();
		}
		else
		{
			VK_CHECK_RESULT(context->getDevice().waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(context->getDevice().resetFences(1, &cmdBuffers[handle].fence));
		}
	}
	else
	{
		// ensure that the cmd buffer is finished with before creating a new one
		// TODO: this could slow things down if we have to wait for cmd bufefrs to finish before
		// continuing so instead have two or three buffers per handle and switch between them
		VK_CHECK_RESULT(context->getDevice().waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(context->getDevice().resetFences(1, &cmdBuffers[handle].fence));

		// create a new buffer - the detructors will worry about destroying everything
		// or just reset?
		if (mode == NewFrameMode::New)
		{
			cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(context);
		}
		else
		{
			if (cmdBuffers[handle].cmdBuffer == nullptr)
			{
				cmdBuffers[handle].cmdBuffer =
				    std::make_unique<CommandBuffer>(context, CommandBuffer::UsageType::Multi);
				cmdBuffers[handle].cmdBuffer->createPrimary();
			}
			else
			{
				cmdBuffers[handle].cmdBuffer.reset({});
			}
		}
	}

	return cmdBuffers[handle].cmdBuffer;
}


void CmdBufferManager::submitFrame(Swapchain& swapchain)
{
	vk::Semaphore waitSync;
	vk::Semaphore signalSync;

	// begin the start of the frame by beginning the next new swapchain image
	swapchain.begin_frame(beginSemaphore);

	uint32_t frameIndex = swapchain.getImageIndex();

	for (uint32_t i = 0; i <= cmdBuffers.size(); ++i)
	{

		vk::CommandBuffer cmdBuffer;
		vk::Fence fence;

		// work out the signalling and wait semaphores
		if (i == 0)
		{
			waitSync = beginSemaphore;
			signalSync = cmdBuffers[i].semaphore;
			cmdBuffer = cmdBuffers[i].cmdBuffer->get();
			fence = cmdBuffers[i].fence;
		}
		else if (i == cmdBuffers.size())
		{
			waitSync = cmdBuffers[i - 1].semaphore;
			signalSync = finalSemaphore;
			cmdBuffer = presentionCmdBuffers[frameIndex].cmdBuffer->get();
			fence = presentionCmdBuffers[frameIndex].fence;
		}
		else
		{
			waitSync = cmdBuffers[i - 1].semaphore;
			signalSync = cmdBuffers[i].semaphore;
			cmdBuffer = cmdBuffers[i].cmdBuffer->get();
			fence = cmdBuffers[i].fence;
		}

		VK_CHECK_RESULT(context->getDevice().resetFences(1, &fence));
		context->getQueue(VkContext::QueueType::Graphics).submitCmdBuffer(cmdBuffer, waitSync, signalSync, fence);
	}

	// then the presentation part.....
	swapchain.submitFrame(finalSemaphore, context->getQueue(VkContext::QueueType::Present).get());
}


}    // namespace VulkanAPI
