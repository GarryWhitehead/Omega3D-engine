#include "CommandBufferManager.h"

#include "VulkanAPI/Managers/SemaphoreManager.h"
#include "VulkanAPI/Renderpass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkDriver.h"

namespace VulkanAPI
{

CmdBufferManager::CmdBufferManager(VkDriver& driver)
    : driver(driver)
{
}

CmdBufferManager::~CmdBufferManager()
{
}

Pipeline* CmdBufferManager::findOrCreatePipeline(const ShaderHandle handle, RenderPass* rPass)
{
    Pipeline* pline = nullptr;

	auto iter = pipelines.find({ handle, rPass });
	
    // if the pipeline has already has an instance return this
    if (iter != pipelines.end())
	{
		pline = &(*iter);
	}
	else
    {
        // else create a new pipeline - If we are in a threaded environemt then we can't add to the list until we are out of the thread
        pline->build(handle, rPass);
    }
    
    return pline;
}

DescriptorSet* findOrCreateDescrSer(const ShaderHandle handle)
{
    
}

CmdBufferHandle CmdBufferManager::newInstance(const CmdBuffer::CmdBufferType type, const uint32_t queueIndex)
{
	CommandBufferInfo bufferInfo;

	bufferInfo.cmdBuffer = std::make_unique<CmdBuffer>(driver, type, queueIndex, *this);
	bufferInfo.queueIndex = queueIndex;

	// create a fence which will be used to sync things
	vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
	VK_CHECK_RESULT(driver.getDevice().createFence(&fence_info, nullptr, &bufferInfo.fence));
	VK_CHECK_RESULT(driver.getDevice().resetFences(1, &bufferInfo.fence));

	// and the semaphore used to sync between queues
	bufferInfo.semaphore = semaphoreManager->getSemaphore();

	cmdBuffers.emplace_back(std::move(bufferInfo));

	return cmdBuffers.size() - 1;
}

std::unique_ptr<CmdBuffer>& CmdBufferManager::getCmdBuffer(CmdBufferHandle handle)
{
	assert(handle < cmdBuffers.size());
	return cmdBuffers[handle].cmdBuffer;
}

std::unique_ptr<VulkanAPI::CmdBuffer>& CmdBufferManager::beginNewFame(CmdBufferHandle handle)
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
			VK_CHECK_RESULT(driver->getDevice().waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
			VK_CHECK_RESULT(driver->getDevice().resetFences(1, &cmdBuffers[handle].fence));
		}
	}
	else
	{
		// ensure that the cmd buffer is finished with before creating a new one
		// TODO: this could slow things down if we have to wait for cmd bufefrs to finish before
		// continuing so instead have two or three buffers per handle and switch between them
		VK_CHECK_RESULT(driver->getDevice().waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(driver->getDevice().resetFences(1, &cmdBuffers[handle].fence));

		// create a new buffer - the detructors will worry about destroying everything
		// or just reset?
		if (mode == NewFrameMode::New)
		{
			cmdBuffers[handle].cmdBuffer = std::make_unique<CommandBuffer>(driver);
		}
		else
		{
			if (cmdBuffers[handle].cmdBuffer == nullptr)
			{
				cmdBuffers[handle].cmdBuffer =
				    std::make_unique<CommandBuffer>(driver, CommandBuffer::UsageType::Multi);
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

		VK_CHECK_RESULT(driver->getDevice().resetFences(1, &fence));
		driver->getQueue(VkDriver::QueueType::Graphics).submitCmdBuffer(cmdBuffer, waitSync, signalSync, fence);
	}

	// then the presentation part.....
	swapchain.submitFrame(finalSemaphore, driver->getQueue(VkDriver::QueueType::Present).get());
}


}    // namespace VulkanAPI
