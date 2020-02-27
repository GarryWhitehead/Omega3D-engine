#include "CBufferManager.h"

#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

// ===============================================================================================================



void CmdPool::submitAll(
    Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore)
{
    vk::Semaphore waitSync;
    vk::Semaphore signalSync;

    for (uint32_t i = 0; i <= cmdInstances.size(); ++i)
    {
        vk::CommandBuffer cmdBuffer;
        vk::Fence fence;

        // work out the signalling and wait semaphores
        if (i == 0)
        {
            waitSync = beginSemaphore;
            signalSync = cmdInstances[i].semaphore;
            cmdBuffer = cmdInstances[i].cmdBuffer->get();
            fence = cmdInstances[i].fence;
        }
        else
        {
            waitSync = cmdInstances[i - 1].semaphore;
            signalSync = cmdInstances[i].semaphore;
            cmdBuffer = cmdInstances[i].cmdBuffer->get();
            fence = cmdInstances[i].fence;
        }

        VK_CHECK_RESULT(context.getVkState().device.resetFences(1, &fence));

        vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eAllCommands;
        vk::SubmitInfo info {1, &waitSync, &flags, 1, &cmdBuffer, 1, &signalSync};
        VK_CHECK_RESULT(context.getGraphQueue().submit(1, &info, fence));
    }

    // then the presentation part.....
    vk::Semaphore finalSemaphore = cmdInstances.back().semaphore;
    vk::PresentInfoKHR presentInfo {1, &finalSemaphore, 1, &swapchain.get(), &imageIndex, nullptr};
    VK_CHECK_RESULT(context.getPresentQueue().presentKHR(&presentInfo));
}

void CmdPool::clearSecondary()
{
    secondary.clear();
}

std::vector<vk::CommandBuffer> CmdPool::getSecondary()
{
    std::vector<vk::CommandBuffer> ret;
    for (auto& sec : secondary)
    {
        ret.emplace_back(sec->get());
    }
    return ret;
}



// ================================================================================================================

CBufferManager::CBufferManager(VkContext& context, SemaphoreManager& spManager)
    : context(context), spManager(spManager)
{
    assert(context.getVkState().device);

    // create the main cmd pool for this buffer
    vk::CommandPoolCreateInfo createInfo {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                          queueIndex};
    context.getVkState().device.createCommandPool(&createInfo, nullptr, &cmdPool);
}

CBufferManager::~CBufferManager()
{
    context.getVkState().device.destroy(cmdPool, nullptr);
}

Pipeline* CBufferManager::findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass)
{
    Pipeline* pline = nullptr;

    PLineHash key {prog, rPass};
    auto iter = pipelines.find(key);

    // if the pipeline has already has an instance return this
    if (iter != pipelines.end())
    {
        pline = iter->second;
    }
    else
    {
        // else create a new pipeline - If we are in a threaded environemt then we can't add to the
        // list until we are out of the thread
        pline = new Pipeline(context, *rPass, *prog->getPLineLayout());
        pline->create(*prog);
        pipelines.emplace(key, pline);
    }

    return pline;
}

void CBufferManager::beginRenderpass(
    CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbuffer)
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
        clearValue[1].depthStencil = vk::ClearDepthStencilValue {rpass.depthClear, 0};
    }

    // extents of the frame buffer
    vk::Rect2D extents {{0, 0}, {fbuffer.getWidth(), fbuffer.getHeight()}};

    vk::RenderPassBeginInfo beginInfo {rpass.get(), fbuffer.get(), extents, 1, clearValue};
    cmdBuffer->beginPass(beginInfo, vk::SubpassContents::eInline);

    // use custom defined viewing area - at the moment set to the framebuffer size
    vk::Viewport viewport {
        0.0f,
        0.0f,
        static_cast<float>(fbuffer.getWidth()),
        static_cast<float>(fbuffer.getHeight()),
        0.0f,
        1.0f};

    cmdBuffer->setViewport(viewport);

    vk::Rect2D scissor {
        {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
        {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}};

    cmdBuffer->setScissor(scissor);
}

void CBufferManager::endRenderpass(CmdBuffer* cmdBuffer)
{
    cmdBuffer->endPass();
}

void CBufferManager::submitFrame(
    Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore)
{
    mainPool->submitAll(swapchain, imageIndex, beginSemaphore);
}



} // namespace VulkanAPI
