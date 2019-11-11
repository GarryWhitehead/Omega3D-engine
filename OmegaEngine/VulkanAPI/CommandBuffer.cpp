#include "CommandBuffer.h"

#include "OEMaths/OEMaths.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"

#include "VulkanAPI/VkContext.h"

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


CmdBuffer::CmdBuffer(VkContext& context, const CmdBufferType type, const uint32_t queueIndex,
                     CmdBufferManager& cbManager)
    : device(context.getDevice())
    , queueFamilyIndex(queueIndex)
    , type(type)
	, cbManager(cbManager)
{
}

CmdBuffer::~CmdBuffer()
{
}

void CmdBuffer::createPrimary()
{
	vk::CommandBufferAllocateInfo allocInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);

	VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmdBuffer));

	vk::CommandBufferUsageFlags usageFlags;
	if (usageType == UsageType::Single)
	{
		usageFlags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
	}
	else
	{
		usageFlags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
	}

	vk::CommandBufferBeginInfo beginInfo(usageFlags, 0);
	VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
}

void CmdBuffer::beginRenderpass(vk::RenderPassBeginInfo& beginInfo, bool useSecondary)
{
	// stor the renderpass and framebuffer locally
	renderpass = beginInfo.renderPass;
	framebuffer = beginInfo.framebuffer;

	assert(renderpass);
	assert(framebuffer);

	// set viewport and scissor using values from renderpass. Shoule be overridable too
	viewPort =
	    vk::Viewport((float)beginInfo.renderArea.offset.x, (float)beginInfo.renderArea.offset.y,
	                 (float)beginInfo.renderArea.extent.width, (float)beginInfo.renderArea.extent.height, 0.0f, 1.0f);

	scissor = vk::Rect2D({ { beginInfo.renderArea.offset.x, beginInfo.renderArea.offset.y },
	                       { beginInfo.renderArea.extent.width, beginInfo.renderArea.extent.height } });

	if (!useSecondary)
	{
		cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eInline);
	}
	else
	{
		cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eSecondaryCommandBuffers);
	}
}

void CmdBuffer::beginRenderpass(vk::RenderPassBeginInfo& beginInfo, vk::Viewport& viewPort)
{
	// stor the renderpass and framebuffer locally
	renderpass = beginInfo.renderPass;
	framebuffer = beginInfo.framebuffer;

	assert(renderpass);
	assert(framebuffer);

	this->viewPort = viewPort;

	// use custom defined viewport
	scissor = vk::Rect2D{ { static_cast<int32_t>(viewPort.x), static_cast<int32_t>(viewPort.y) },
		                  { static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height) } };

	cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eInline);
}

void CmdBuffer::endRenderpass()
{
	cmdBuffer.endRenderPass();
}

void CmdBuffer::end()
{
	cmdBuffer.end();
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
	assert(!secondarys.empty());

	// zero value indicates to execute all cmd buffers
	if (count == 0)
	{
		count = secondarys.size();
	}
	assert(count < secondarys.size());

	// trasnfer all the secondary cmd buffers into a container for execution
	std::vector<vk::CommandBuffer> executeCmdBuffers(secondarys.size());
	for (uint32_t i = 0; i < secondarys.size(); ++i)
	{
		executeCmdBuffers[i] = secondarys[i].get();
	}

	cmdBuffer.executeCommands(static_cast<uint32_t>(executeCmdBuffers.size()), executeCmdBuffers.data());
}

CmdBuffer& CmdBuffer::createSecondary()
{
	CmdBuffer secondary = *this;
	secondary.create();
	secondarys.emplace_back(secondary);
	return secondarys.back();
}

void CmdBuffer::createSecondary(uint32_t count)
{
	secondarys.resize(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		secondarys[i] = { device, queueFamilyIndex, renderpass, framebuffer, viewPort, scissor,  };
		secondarys[i].create();
	}
}

// drawing functions
void CmdBuffer::drawIndexed(uint32_t indexCount)
{
	cmdBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void CmdBuffer::drawQuad()
{
	cmdBuffer.draw(3, 1, 0, 0);
}


// command pool functions =====================================================================

void CommandBuffer::createCmdPool()
{
	vk::CommandPoolCreateInfo createInfo(usageType == UsageType::Single ?
	                                         vk::CommandPoolCreateFlagBits::eTransient :
	                                         vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	                                     queueFamilyIndex);

	device.createCommandPool(&createInfo, nullptr, &cmdPool);
}

}    // namespace VulkanAPI
