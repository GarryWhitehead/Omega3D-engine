#include "CommandBuffer.h"

#include "OEMaths/OEMaths.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkContext.h"

#include <cassert>

namespace VulkanAPI
{

CmdBuffer::CmdBuffer(VkContext& context, vk::CommandPool& cmdPool, const Type type) : context(context), cmdPool(cmdPool), type(type)
{
}

CmdBuffer::~CmdBuffer()
{
}

void CmdBuffer::init()
{
    // create the fence
    vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlagBits(0));
    VK_CHECK_RESULT(context.device.createFence(&fenceInfo, nullptr, &cmdFence));
    VK_CHECK_RESULT(context.device.resetFences(1, &cmdFence));

    vk::CommandBufferLevel level = type == Type::Primary ? vk::CommandBufferLevel::ePrimary
                                                         : vk::CommandBufferLevel::eSecondary;
    vk::CommandBufferAllocateInfo allocInfo(cmdPool, level, 1);
    VK_CHECK_RESULT(context.device.allocateCommandBuffers(&allocInfo, &cmdBuffer));
}

void CmdBuffer::begin()
{
    vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
    vk::CommandBufferBeginInfo beginInfo(usageFlags, nullptr);
    VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
}

void CmdBuffer::beginSecondary(RenderPass& renderpass)
{
    assert(type == CmdBuffer::Type::Secondary);
    
    // the secondary commands inherits from the primary buffer
    vk::CommandBufferInheritanceInfo inheritance {renderpass.get(), 0, renderpass.getFrameBuffer(), 0, {}, {}};
    
    vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
    vk::CommandBufferBeginInfo beginInfo(usageFlags, &inheritance);
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

void CmdBuffer::end()
{
    cmdBuffer.end();
}

void CmdBuffer::setScissor(const vk::Rect2D& newScissor)
{
    scissor = newScissor;
    cmdBuffer.setScissor(0, 1, &scissor);
}

void CmdBuffer::setViewport(const vk::Viewport& newViewPort)
{
    viewPort = newViewPort;
    scissor = vk::Rect2D {
        {0, 0}, {static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height)}};
    cmdBuffer.setViewport(0, 1, &viewPort);
}

void CmdBuffer::bindPipeline(CBufferManager& cbManager, RenderPass* renderpass, ShaderProgram* program)
{
    Pipeline* pline = cbManager.findOrCreatePipeline(program, renderpass);
    assert(pline);
    // check whether this pipeline is already bound - we don't have to do anything if so
    if (!boundPipeline || pline != boundPipeline)
    {
        vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(pline->getType());
        cmdBuffer.bindPipeline(bindPoint, pline->get());
        boundPipeline = pline;
    }
}

void CmdBuffer::bindDescriptors(CBufferManager& cbManager, ShaderProgram* prog, const Pipeline::Type pipelineType)
{
    vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(pipelineType);
    std::vector<DescriptorSetInfo> setInfo = cbManager.findDescriptorSets(prog->getShaderId());
    assert(setInfo.empty());
    
    std::vector<vk::DescriptorSet> descrSets(setInfo.size());
    for (uint64_t i = 0; i < descrSets.size(); ++i)
    {
        descrSets[i] = setInfo[i].descrSet;
    }
    
    cmdBuffer.bindDescriptorSets(
        bindPoint,
        prog->pLineLayout->get(),
        0,
        static_cast<uint32_t>(descrSets.size()),
        descrSets.data(),
        0,
        nullptr);
}

void CmdBuffer::bindDynamicDescriptors(CBufferManager& cbManager,
    ShaderProgram* prog, std::vector<uint32_t>& offsets, const Pipeline::Type pipelineType)
{
    vk::PipelineBindPoint bindPoint = Pipeline::createBindPoint(pipelineType);
    std::vector<DescriptorSetInfo> setInfo = cbManager.findDescriptorSets(prog->getShaderId());
    assert(setInfo.empty());
    
    std::vector<vk::DescriptorSet> descrSets(setInfo.size());
    for (uint64_t i = 0; i < descrSets.size(); ++i)
    {
        descrSets[i] = setInfo[i].descrSet;
    }
    
    cmdBuffer.bindDescriptorSets(
        bindPoint,
        prog->pLineLayout->get(),
        0,
        static_cast<uint32_t>(descrSets.size()),
        descrSets.data(),
        static_cast<uint32_t>(offsets.size()),
        offsets.data());
}

void CmdBuffer::bindDynamicDescriptors(CBufferManager& cbManager,
    ShaderProgram* prog, const uint32_t offset, const Pipeline::Type pipelineType)
{
    std::vector<uint32_t> offsets = {offset};
    bindDynamicDescriptors(cbManager, prog, offsets, pipelineType);
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

    vk::Queue queue = context.graphicsQueue;
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr);

    VK_CHECK_RESULT(queue.submit(1, &submitInfo, cmdFence));
    
    // let the user know that the cmd buffer has work to do
    workSubmitted = true;
}

void CmdBuffer::submit(
    vk::Semaphore& waitSemaphore, vk::Semaphore& signalSemaphore, vk::Fence& fence)
{
    vk::Queue queue = context.graphicsQueue;
    vk::PipelineStageFlags stageFlag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submitInfo(1, &waitSemaphore, &stageFlag, 1, &cmdBuffer, 1, &signalSemaphore);

    VK_CHECK_RESULT(queue.submit(1, &submitInfo, fence));
}

void CmdBuffer::resetPool()
{
    VK_CHECK_RESULT(context.device.waitForFences(1, &cmdFence, VK_TRUE, UINT64_MAX));
    VK_CHECK_RESULT(context.device.resetFences(1, &cmdFence));

    context.device.resetCommandPool(cmdPool, static_cast<vk::CommandPoolResetFlagBits>(0));
}

void CmdBuffer::resetCmdBuffer()
{
    cmdBuffer.reset(vk::CommandBufferResetFlags(0));
}

} // namespace VulkanAPI
