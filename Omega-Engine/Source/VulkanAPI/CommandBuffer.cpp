#include "CommandBuffer.h"

#include "OEMaths/OEMaths.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkContext.h"

#include <cassert>

namespace VulkanAPI
{

CmdBuffer::CmdBuffer(
    VkContext& context, const Type type, CmdPool& cmdPool, CmdBufferManager* cbManager)
    : context(context), cmdPool(cmdPool), cbManager(cbManager), type(type)
{
    alloc();
}

CmdBuffer::~CmdBuffer()
{
}

void CmdBuffer::alloc()
{
    vk::CommandBufferLevel level = type == Type::Primary ? vk::CommandBufferLevel::ePrimary
                                                         : vk::CommandBufferLevel::eSecondary;
    vk::CommandBufferAllocateInfo allocInfo(cmdPool.get(), level, 1);

    VK_CHECK_RESULT(context.getDevice().allocateCommandBuffers(&allocInfo, &cmdBuffer));
}

void CmdBuffer::begin()
{    
    vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
    vk::CommandBufferBeginInfo beginInfo(usageFlags, 0);
    VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
}

void CmdBuffer::beginPass(
    const vk::RenderPassBeginInfo& beginInfo, const vk::SubpassContents contents)
{
    cmdBuffer.beginRenderPass(beginInfo, contents);
}

void CmdBuffer::endPass()
{
    cmdBuffer.endRenderPass();
}

void CmdBuffer::setScissor(const vk::Rect2D& scissor)
{
    this->scissor = scissor;
    cmdBuffer.setScissor(0, 1, &scissor);
}

void CmdBuffer::setViewport(const vk::Viewport& viewPort)
{
    this->viewPort = viewPort;
    scissor = vk::Rect2D {
        {0, 0}, {static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height)}};
    cmdBuffer.setViewport(0, 1, &viewPort);
}

void CmdBuffer::bindPipeline(RenderPass* renderpass, ShaderProgram* program)
{
    Pipeline* pline = cbManager->findOrCreatePipeline(program, renderpass);
    assert(pline);
    // check whether this pipeline is already bound - we don't have to do anything if so
    if (!boundPipeline || pline != boundPipeline)
    {
        vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(pline->getType());
        cmdBuffer.bindPipeline(bindPoint, pline->get());
        boundPipeline = pline;
    }
}

void CmdBuffer::bindDescriptors(ShaderProgram* prog, const Pipeline::Type type)
{
    DescriptorSet* set = prog->descrSet.get();
    if (!boundDescrSet || set != boundDescrSet)
    {
        vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(type);
        std::vector<vk::DescriptorSet> descrSets = set->getOrdered();
        cmdBuffer.bindDescriptorSets(
            bindPoint,
            prog->pLineLayout->get(),
            0,
            static_cast<uint32_t>(descrSets.size()),
            descrSets.data(),
            0,
            nullptr);
        boundDescrSet = set;
    }
}

void CmdBuffer::bindDynamicDescriptors(
    ShaderProgram* prog, std::vector<uint32_t>& offsets, const Pipeline::Type type)
{
    DescriptorSet* set = prog->descrSet.get();
    assert(set);
    if (!boundDescrSet || set != boundDescrSet)
    {
        vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(type);
        std::vector<vk::DescriptorSet> descrSets = set->getOrdered();
        cmdBuffer.bindDescriptorSets(
            bindPoint,
            prog->pLineLayout->get(),
            0,
            static_cast<uint32_t>(descrSets.size()),
            descrSets.data(),
            static_cast<uint32_t>(offsets.size()),
            offsets.data());
        boundDescrSet = set;
    }
}

void CmdBuffer::bindDynamicDescriptors(
    ShaderProgram* prog, const uint32_t offset, const Pipeline::Type type)
{
    std::vector<uint32_t> offsets = {offset};
    bindDynamicDescriptors(prog, offsets, type);
}

void CmdBuffer::bindPushBlock(
    ShaderProgram* prog, vk::ShaderStageFlags stage, uint32_t size, void* data)
{
    cmdBuffer.pushConstants(prog->pLineLayout->get(), stage, 0, size, data);
}

void CmdBuffer::setDepthBias(float biasConstant, float biasClamp, float biasSlope)
{
    cmdBuffer.setDepthBias(biasConstant, biasClamp, biasSlope);
}

void CmdBuffer::bindVertexBuffer(vk::Buffer buffer, vk::DeviceSize offset)
{
    cmdBuffer.bindVertexBuffers(0, 1, &buffer, &offset);
}

void CmdBuffer::bindIndexBuffer(vk::Buffer buffer, uint32_t offset)
{
    cmdBuffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
}

// ================= drawing functions =========================
void CmdBuffer::drawIndexed(uint32_t indexCount)
{
    cmdBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void CmdBuffer::drawIndexed(uint32_t indexCount, int32_t offset)
{
    cmdBuffer.drawIndexed(indexCount, 1, 0, offset, 0);
}

void CmdBuffer::drawQuad()
{
    cmdBuffer.draw(3, 1, 0, 0);
}

// ================= queue functions ============================
void CmdBuffer::flush()
{
    // end the command buffer before flushing the stream
    cmdBuffer.end();

    vk::Queue queue = context.getGraphQueue();
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr);

    VK_CHECK_RESULT(queue.submit(1, &submitInfo, {}));
    // queue.waitIdle();
}

void CmdBuffer::submit(
    vk::Semaphore& waitSemaphore, vk::Semaphore& signalSemaphore, vk::Fence& fence)
{
    vk::Queue queue = context.getGraphQueue();
    vk::PipelineStageFlags stageFlag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo(1, &waitSemaphore, &stageFlag, 1, &cmdBuffer, 1, &signalSemaphore);

    VK_CHECK_RESULT(queue.submit(1, &submitInfo, fence));
    // queue.waitIdle();
}


} // namespace VulkanAPI
