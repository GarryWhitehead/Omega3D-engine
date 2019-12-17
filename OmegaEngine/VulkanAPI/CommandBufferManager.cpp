#include "CommandBufferManager.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Renderpass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"

#include "utility/Logger.h"

namespace VulkanAPI
{

// ===============================================================================================================

CmdPool::CmdPool(VkContext& context, int queueIndex)
    : queueIndex(queueIndex)
    , context(context)
{
	// create the cmd pool - one per cmd buffer
	vk::CommandPoolCreateInfo createInfo{ vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex };
	context.getDevice().createCommandPool(&createInfo, nullptr, &cmdPool);
}

CmdPool::~CmdPool()
{
	context.getDevice().destroy(cmdPool, nullptr);
}

CmdBuffer* CmdPool::createCmdBuffer()
{
	CmdPool::CmdInstance instance;

	// create a fence which will be used to sync things
	vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
	VK_CHECK_RESULT(context.getDevice().createFence(&fence_info, nullptr, &instance.fence));
	VK_CHECK_RESULT(context.getDevice().resetFences(1, &instance.fence));

	// and the semaphore used to sync between queues
	instance.semaphore = spManager.getSemaphore();

	// and create the cmd buffer
	instance.cmdBuffer = std::make_unique<CmdBuffer>(context, CmdBuffer::Type::Primary, queueIndex, *this);
	CmdBuffer* res = instance.cmdBuffer.get();
	cmdInstances.emplace_back(std::move(instance));

	return res;
}

void CmdPool::reset()
{
	if (cmdInstances.empty())
	{
		LOGGER_WARN("Calling reset on a command pool which has no cmd buffers allocated!");
		return;
	}

	// ensure all cmd buffers are finished fror this frame before restting the pool
	for (CmdInstance& info : cmdInstances)
	{
		VK_CHECK_RESULT(context.getDevice().waitForFences(1, &info.fence, VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(context.getDevice().resetFences(1, &info.fence));
	}

	context.getDevice().resetCommandPool(cmdPool, static_cast<vk::CommandPoolResetFlagBits>(0));
}

// ================================================================================================================

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

	PLineHash key{ prog, rPass };
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
		pline->create(context, *rPass, *prog);
		pipelines.emplace(key, pline);
	}

	return pline;
}

CmdPool* CmdBufferManager::createPool()
{
	auto pool =
	    std::make_unique<CmdPool>(context, context.getGraphQueueIdx());    // this needs to support compute queues too
	assert(pool);
	CmdPool* res = pool.get();
	cmdPools.emplace_back(std::move(pool));
	return res;
}

void CmdBufferManager::beginNewFame()
{
	vk::Device dev = context.getDevice();

	for (auto& pool : cmdPools)
	{
		pool->reset();
	}
}

void CmdBufferManager::beginRenderpass(CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbuffer)
{
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
	vk::Extent2D extents{ fbuffer.getWidth(), fbuffer.getHeight() };

	vk::RenderPassBeginInfo beginInfo{ rpass.get(), fbuffer.get(), extents, 1, &clearValue };
	cmdBuffer->beginRenderPass(&beginInfo, vk::SubpassContents::eInline);

	// use custom defined viewing area - at the moment set to the framebuffer size
	vk::Viewport viewport{ 0.0f, 0.0f, fbuffer.getWidth(), fbuffer.getHeight(), 0.0f, 1.0f };

	cmdBuffer->setViewport(viewport);

	vk::Rect2D scissor{ { static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y) },
		                { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) } };

	cmdBuffer->setScissor(scissor);
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
