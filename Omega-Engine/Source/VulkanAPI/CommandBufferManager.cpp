#include "CommandBufferManager.h"

#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

// ===============================================================================================================

CmdPool::CmdPool(VkContext& context, CmdBufferManager* cbManager, SemaphoreManager& spManager, uint32_t queueIndex)
    : context(context), spManager(spManager), cbManager(cbManager), queueIndex(queueIndex)
{
    assert(cbManager);

    vk::CommandPoolCreateInfo createInfo {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueIndex};
    context.getDevice().createCommandPool(&createInfo, nullptr, &cmdPool);

    // create the required number of single submit cmd buffers
    singleUseCbs.resize(SingleUseCbSize);
    for (uint8_t i = 0; i < SingleUseCbSize; ++i)
    {
        auto cmdBuffer =
            std::make_unique<CmdBuffer>(context, CmdBuffer::Type::Primary, *this, cbManager);
        singleUseCbs[i] = std::move(cmdBuffer);
    }
}

CmdPool::~CmdPool()
{
    context.getDevice().destroy(cmdPool, nullptr);
}

CmdBuffer* CmdPool::createPrimaryCmdBuffer()
{
    CmdPool::CmdInstance instance;

    // create a fence which will be used to sync things
    vk::FenceCreateInfo fence_info(vk::FenceCreateFlagBits(0));
    VK_CHECK_RESULT(context.getDevice().createFence(&fence_info, nullptr, &instance.fence));
    VK_CHECK_RESULT(context.getDevice().resetFences(1, &instance.fence));

    // and the semaphore used to sync between queues
    instance.semaphore = spManager.getSemaphore();

    // and create the cmd buffer
    instance.cmdBuffer =
        std::make_unique<CmdBuffer>(context, CmdBuffer::Type::Primary, *this, cbManager);
    CmdBuffer* res = instance.cmdBuffer.get();
    cmdInstances.emplace_back(std::move(instance));

    return res;
}

CmdBuffer* CmdPool::createSecCmdBuffer()
{
    // and create the secondary cmd buffer
    auto cmdBuffer =
        std::make_unique<CmdBuffer>(context, CmdBuffer::Type::Secondary, *this, cbManager);
    CmdBuffer* res = cmdBuffer.get();
    secondary.emplace_back(std::move(cmdBuffer));
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

        VK_CHECK_RESULT(context.getDevice().resetFences(1, &fence));

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

std::unique_ptr<CmdBuffer> CmdPool::getSingleUseCb()
{
    // TODO: should check here that the cmd buffer has actually finished its work before returning
    // to the user
    // TODO: using a deque here, probably expensive reagrds performance so find a better way
    // We remove from the front and add to the back. This way we can be more certain that cmd buffer will have finished their tasks
    if (!singleUseCbs.empty())
    {
        std::unique_ptr<CmdBuffer> cb = std::move(singleUseCbs.front());
        cb->begin();
        singleUseCbs.pop_front();
        return std::move(cb);
    }

    auto cmdBuffer =
        std::make_unique<CmdBuffer>(context, CmdBuffer::Type::Primary, *this, cbManager);
    return cmdBuffer;
}

void CmdPool::releaseSingleUseCb(std::unique_ptr<CmdBuffer> cmdBuffer)
{
    assert(cmdBuffer);
    singleUseCbs.emplace_back(std::move(cmdBuffer));
}

// ================================================================================================================

CmdBufferManager::CmdBufferManager(VkContext& context)
    : context(context), spManager(std::make_unique<SemaphoreManager>(context.getDevice()))
{
    assert(context.getDevice());
    createMainPool();
}

CmdBufferManager::~CmdBufferManager()
{
}

Pipeline* CmdBufferManager::findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass)
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

CmdPool* CmdBufferManager::createMainPool()
{
    mainPool = std::make_unique<CmdPool>(
        context,
        this,
        *spManager,
        context.getGraphQueueIdx()); // this needs to support compute queues too
    return mainPool.get();
}

std::unique_ptr<CmdPool> CmdBufferManager::createSecondaryPool()
{
    // if there are any free pools which are intended for threaded use, then return one of these if
    // applicable
    if (!secondaryPools.empty())
    {
        auto& pool = secondaryPools.back();
        secondaryPools.pop_back();
        return std::move(pool);
    }

    // create a new pool if none are free
    auto pool = std::make_unique<CmdPool>(
        context,
        this,
        *spManager,
        context.getGraphQueueIdx()); // this needs to support compute queues too
    return pool;
}

void CmdBufferManager::releasePool(std::unique_ptr<CmdPool> pool)
{
    assert(pool);

    // destroy the cmd buffers - these will be allocated each frame - TODO: maybe these should be
    // recycled?
    pool->clearSecondary();
    pool->reset();
    secondaryPools.emplace_back(std::move(pool));
}

void CmdBufferManager::beginNewFame()
{
    mainPool->reset();
}

void CmdBufferManager::beginRenderpass(
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

void CmdBufferManager::endRenderpass(CmdBuffer* cmdBuffer)
{
    cmdBuffer->endPass();
}

void CmdBufferManager::submitFrame(
    Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore)
{
    mainPool->submitAll(swapchain, imageIndex, beginSemaphore);
}

std::unique_ptr<CmdBuffer> CmdBufferManager::getSingleUseCb()
{
    return mainPool->getSingleUseCb();
}

void CmdBufferManager::releaseSingleUseCb(std::unique_ptr<CmdBuffer> cmdBuffer)
{
    mainPool->releaseSingleUseCb(std::move(cmdBuffer));
}

} // namespace VulkanAPI
