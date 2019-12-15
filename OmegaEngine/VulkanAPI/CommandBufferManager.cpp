#include "CommandBufferManager.h"

#include "VulkanAPI/SemaphoreManager.h"
#include "VulkanAPI/Renderpass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Pipeline.h"

namespace VulkanAPI
{

CmdBufferManager::CmdBufferManager(VkContext& context)
    : context(context)
{
}

CmdBufferManager::~CmdBufferManager()
{
}

Pipeline* CmdBufferManager::findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass)
{
    Pipeline* pline = nullptr;

    PLineHash key { prog, rPass };
	auto iter = pipelines.find(key);
	
    // if the pipeline has already has an instance return this
    if (iter != pipelines.end())
	{
		pline = &(*iter);
	}
	else
    {
        // else create a new pipeline - If we are in a threaded environemt then we can't add to the list until we are out of the thread
        pline = new Pipeline();
        pline->create(context.getDevice(), *rPass, *prog);
        pipelines.emplace(key, pline);
    }
    
    return pline;
}

CmdBufferHandle CmdBufferManager::createCmdBuffer()
{
    CmdBufferInfo bufferInfo;
	vk::Device dev = context.getDevice();

	bufferInfo.queueIndex = queueIndex;

	// create a fence which will be used to sync things
	vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
	VK_CHECK_RESULT(dev.createFence(&fence_info, nullptr, &bufferInfo.fence));
	VK_CHECK_RESULT(dev.resetFences(1, &bufferInfo.fence));

	// and the semaphore used to sync between queues
	bufferInfo.semaphore = semaphoreManager->getSemaphore();

	// create the cmd pool - one per cmd buffer
	vk::CommandPoolCreateInfo createInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndx };
	dev.createCommandPool(&createInfo, nullptr, &bufferInfo.cmdPool);

	// and create the cmd buffer
	bufferInfo.cmdBuffer = std::make_unique<CmdBuffer>(context, type, queueIndex, *this);

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
	vk::Device dev = context.getDevice();

    // ensure that the cmd buffer is finished with before creating a new one
    // TODO: this could slow things down if we have to wait for cmd bufefrs to finish before
    // continuing so instead have two or three buffers per handle and switch between them
    VK_CHECK_RESULT(dev.waitForFences(1, &cmdBuffers[handle].fence, VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(dev.resetFences(1, &cmdBuffers[handle].fence));

    // create a new buffer - the detructors will worry about destroying everything
    // or just reset?
    if (mode == NewFrameMode::New)
    {
        cmdBuffers[handle].cmdBuffer = std::make_unique<CmdBuffer>(driver);
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
	

	return cmdBuffers[handle].cmdBuffer;
}

void CmdBufferManager::beginRenderpass(const CmdBufferHandle handle, RenderPass& rpass, FrameBuffer& fbuffer)
{
    vk::CommandBuffer cmdBuffer = cmdBuffers[rpass.cmdBufferHandle]->get();
    
    // setup the clear values for this pass - need one for each attachment
    vk::ClearValue clearValue[2];
    
    if (rpass.hasColourAttach())
    {
        clearValue[0].color.float32[0] = rpass.clearCol.r;
        clearValue[0].color.float32[1] = rpass.clearCol.g;
        clearValue[0].color.float32[2] = rpass.clearCol.b;
        clearValue[0].color.float32[3] = rpass.clearCol.a;
    }
    if (rpass.hasDepthAttach())
    {
        clearValue[1].depthStencil = { rpass.depthClear, 0 };
    }
    
    // extents of the frame buffer
    vk::Extent2D extents { fbuffer.width, fbuffer.height };
    
    vk::RenderPassBeginInfo beginInfo { rpass.get(), fbuffer.get(), extents, 1, &clearValue };
    cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eInline);

    // use custom defined viewing area
    vk::Rect2D viewPort { { static_cast<int32_t>(viewPort.x), static_cast<int32_t>(viewPort.y) },
    { static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height) } };
    
    cmdBuffer.setViewPort(0, 1 &viewport);
    
    vk::Rect2D scissor { { static_cast<int32_t>(viewPort.x), static_cast<int32_t>(viewPort.y) },
                          { static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height) } };
    
    cmdBuffer.setScissor(0, 1, & scissor);
}

void CmdBufferManager::endRenderpass(const CmdBufferHandle handle)
{
    vk::CommandBuffer cmdBuffer = cmdBuffers[rpass.cmdBufferHandle]->get();
    cmdBuffer.endRenderPass();
}

void CmdBufferManager::submitAll(Swapchain& swapchain)
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
