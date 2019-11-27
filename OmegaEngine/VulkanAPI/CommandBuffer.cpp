#include "CommandBuffer.h"

#include "OEMaths/OEMaths.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Managers/CommandBufferManager.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/VkContext.h"

#include <cassert>

namespace VulkanAPI
{


vk::PipelineBindPoint createBindPoint(PipelineType type)
{
	vk::PipelineBindPoint bindPoint;

	switch (type)
	{
	case PipelineType::Graphics:
		bindPoint = vk::PipelineBindPoint::eGraphics;
		break;
	case PipelineType::Compute:
		bindPoint = vk::PipelineBindPoint::eCompute;
		break;
	}

	return bindPoint;
}


CmdBuffer::CmdBuffer(VkContext& context, const Type type, vk::CommandPool* cmdPool, CmdBufferManager* cbManager)
    : context(context)
    , type(type)
    , cmdPool(cmdPool)
    , cbManager(cbManager)
{
	if (type == Type::Primary)
	{
		prepare();
	}
}

CmdBuffer::~CmdBuffer()
{
}

void CmdBuffer::prepare()
{
	vk::CommandBufferAllocateInfo allocInfo(*cmdPool, vk::CommandBufferLevel::ePrimary, 1);

	VK_CHECK_RESULT(context.getDevice().allocateCommandBuffers(&allocInfo, &cmdBuffer));

	vk::CommandBufferUsageFlags usageFlags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
	vk::CommandBufferBeginInfo beginInfo(usageFlags, 0);
	VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
}

void CmdBuffer::setViewport()
{
	cmdBuffer.setViewport(0, 1, &viewPort);
}

void CmdBuffer::setScissor()
{
	cmdBuffer.setScissor(0, 1, &scissor);
}

void CmdBuffer::setViewport(const vk::Viewport& viewPort)
{
	this->viewPort = viewPort;
	scissor = vk::Rect2D{ { 0, 0 }, { static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height) } };
	cmdBuffer.setViewport(0, 1, &viewPort);
}

void CmdBuffer::bindPipeline(Pipeline& pipeline)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(pipeline.getPipelineType());
	cmdBuffer.bindPipeline(bindPoint, pipeline.get());
}

void CmdBuffer::bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(bindPoint, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 0,
	                             nullptr);
}

void CmdBuffer::bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, uint32_t offsetCount,
                                uint32_t* offsets, PipelineType type)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(bindPoint, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(),
	                             offsetCount, offsets);
}

void CmdBuffer::bindPushBlock(PipelineLayout& pipelineLayout, vk::ShaderStageFlags stage, uint32_t size, void* data)
{
	cmdBuffer.pushConstants(pipelineLayout.get(), stage, 0, size, data);
}

void CmdBuffer::setDepthBias(float biasConstant, float biasClamp, float biasSlope)
{
	cmdBuffer.setDepthBias(biasConstant, biasClamp, biasSlope);
}

void CmdBuffer::bindVertexBuffer(vk::Buffer& buffer, vk::DeviceSize offset)
{
	cmdBuffer.bindVertexBuffers(0, 1, &buffer, &offset);
}

void CmdBuffer::bindIndexBuffer(vk::Buffer& buffer, uint32_t offset)
{
	cmdBuffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
}

void CmdBuffer::executeSecondary(size_t count)
{
	assert(!secondary.empty());

	// zero value indicates to execute all cmd buffers
	if (count == 0)
	{
		count = secondary.size();
	}
	assert(count < secondary.size());

	// trasnfer all the secondary cmd buffers into a container for execution
	std::vector<vk::CommandBuffer> executeCmdBuffers(secondary.size());
	for (uint32_t i = 0; i < secondary.size(); ++i)
	{
		executeCmdBuffers[i] = secondary[i].get();
	}

	cmdBuffer.executeCommands(static_cast<uint32_t>(executeCmdBuffers.size()), executeCmdBuffers.data());
}

CmdBuffer& CmdBuffer::createSecondary()
{
	CmdBuffer secBuffer = *this;
	secBuffer.prepare();
	secondary.emplace_back(secBuffer);
	return secondary.back();
}

void CmdBuffer::createSecondary(size_t count)
{
	secondary.resize(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		secondary[i] = {
			device, queueFamilyIndex, renderpass, framebuffer, viewPort, scissor,
		};
		secondarys[i].create();
	}
}

// ================= drawing functions =========================
void CmdBuffer::drawIndexed(size_t indexCount)
{
	cmdBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void CmdBuffer::drawQuad()
{
	cmdBuffer.draw(3, 1, 0, 0);
}

// ================= queue functions ============================
void CmdBuffer::flush()
{
	vk::Queue queue = context.getGraphQueue();
	vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr);

	VK_CHECK_RESULT(queue.submit(1, &submitInfo, {}));
	queue.waitIdle();
}

void CmdBuffer::submit(vk::Semaphore& waitSemaphore, vk::Semaphore& signalSemaphore, vk::Fence& fence)
{
	vk::Queue queue = context.getGraphQueue();
	vk::PipelineStageFlags stageFlag = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	vk::SubmitInfo submitInfo(1, &waitSemaphore, &stageFlag, 1, &cmdBuffer, 1, &signalSemaphore);

	VK_CHECK_RESULT(queue.submit(1, &submitInfo, fence));
	queue.waitIdle();
}


}    // namespace VulkanAPI
