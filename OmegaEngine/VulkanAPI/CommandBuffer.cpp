#include "CommandBuffer.h"
#include "OEMaths/OEMaths.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"

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

CommandBuffer::CommandBuffer()
{
}

CommandBuffer::CommandBuffer(vk::Device dev, uint32_t index)
    : device(dev)
    , queueFamilyIndex(index)
    , usageType(UsageType::Single)
{
	// create a cmd pool for this buffer
	createCmdPool();
}

CommandBuffer::CommandBuffer(vk::Device dev, uint32_t index, UsageType type)
    : device(dev)
    , queueFamilyIndex(index)
    , usageType(type)
{
	// create a cmd pool for this buffer
	createCmdPool();
}

CommandBuffer::~CommandBuffer()
{
}

void CommandBuffer::init(vk::Device dev, uint32_t index)
{
	device = dev;
	queueFamilyIndex = index;
	usageType = UsageType::Single;

	// create a cmd pool for this buffer
	createCmdPool();
}

void CommandBuffer::createPrimary()
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

void CommandBuffer::beginRenderpass(vk::RenderPassBeginInfo &beginInfo, bool useSecondary)
{
	// stor the renderpass and framebuffer locally
	renderpass = beginInfo.renderPass;
	framebuffer = beginInfo.framebuffer;

	assert(renderpass);
	assert(framebuffer);

	// set viewport and scissor using values from renderpass. Shoule be overridable too
	viewPort =
	    vk::Viewport((float)beginInfo.renderArea.offset.x, (float)beginInfo.renderArea.offset.y,
	                 (float)beginInfo.renderArea.extent.width,
	                 (float)beginInfo.renderArea.extent.height, 0.0f, 1.0f);

	scissor =
	    vk::Rect2D({ { beginInfo.renderArea.offset.x, beginInfo.renderArea.offset.y },
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

void CommandBuffer::beginRenderpass(vk::RenderPassBeginInfo &beginInfo, vk::Viewport &viewPort)
{
	// stor the renderpass and framebuffer locally
	renderpass = beginInfo.renderPass;
	framebuffer = beginInfo.framebuffer;

	assert(renderpass);
	assert(framebuffer);

	this->viewPort = viewPort;

	// use custom defined viewport
	scissor = vk::Rect2D{ { static_cast<int32_t>(viewPort.x), static_cast<int32_t>(viewPort.y) },
		                  { static_cast<uint32_t>(viewPort.width),
		                    static_cast<uint32_t>(viewPort.height) } };

	cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eInline);
}

void CommandBuffer::endRenderpass()
{
	cmdBuffer.endRenderPass();
}

void CommandBuffer::end()
{
	cmdBuffer.end();
}

void CommandBuffer::setViewport()
{
	cmdBuffer.setViewport(0, 1, &viewPort);
}

void CommandBuffer::setScissor()
{
	cmdBuffer.setScissor(0, 1, &scissor);
}

void CommandBuffer::setViewport(const vk::Viewport &viewPort)
{
	this->viewPort = viewPort;
	scissor = vk::Rect2D{
		{ 0, 0 }, { static_cast<uint32_t>(viewPort.width), static_cast<uint32_t>(viewPort.height) }
	};
	cmdBuffer.setViewport(0, 1, &viewPort);
}

void CommandBuffer::bindPipeline(Pipeline &pipeline)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(pipeline.getPipelineType());
	cmdBuffer.bindPipeline(bindPoint, pipeline.get());
}

void CommandBuffer::bindDescriptors(PipelineLayout &pipelineLayout, DescriptorSet &descriptorSet,
                                    PipelineType type)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(bindPoint, pipelineLayout.get(), 0,
	                             static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void CommandBuffer::bindDescriptors(PipelineLayout &pipelineLayout, DescriptorSet &descriptorSet,
                                    uint32_t offsetCount, uint32_t *offsets, PipelineType type)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(bindPoint, pipelineLayout.get(), 0,
	                             static_cast<uint32_t>(sets.size()), sets.data(), offsetCount,
	                             offsets);
}

void CommandBuffer::bindPushBlock(PipelineLayout &pipelineLayout, vk::ShaderStageFlags stage,
                                  uint32_t size, void *data)
{
	cmdBuffer.pushConstants(pipelineLayout.get(), stage, 0, size, data);
}

void CommandBuffer::setDepthBias(float biasConstant, float biasClamp, float biasSlope)
{
	cmdBuffer.setDepthBias(biasConstant, biasClamp, biasSlope);
}

void CommandBuffer::bindVertexBuffer(vk::Buffer &buffer, vk::DeviceSize offset)
{
	cmdBuffer.bindVertexBuffers(0, 1, &buffer, &offset);
}

void CommandBuffer::bindIndexBuffer(vk::Buffer &buffer, uint32_t offset)
{
	cmdBuffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
}

void CommandBuffer::executeSecondaryCommands()
{
	assert(!secondaryCmdBuffers.empty());

	// trasnfer all the secondary cmd buffers into a container for execution
	std::vector<vk::CommandBuffer> executeCmdBuffers(secondaryCmdBuffers.size());
	for (uint32_t i = 0; i < secondaryCmdBuffers.size(); ++i)
	{
		executeCmdBuffers[i] = secondaryCmdBuffers[i].get();
	}

	cmdBuffer.executeCommands(static_cast<uint32_t>(executeCmdBuffers.size()),
	                          executeCmdBuffers.data());
}

void CommandBuffer::executeSecondaryCommands(uint32_t count)
{
	assert(!secondaryCmdBuffers.empty());

	// trasnfer the specified amount of secondary cmd buffers into a container for execution
	std::vector<vk::CommandBuffer> executeCmdBuffers(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		executeCmdBuffers[i] = secondaryCmdBuffers[i].get();
	}

	cmdBuffer.executeCommands(count, executeCmdBuffers.data());
}

SecondaryCommandBuffer &CommandBuffer::createSecondary()
{
	SecondaryCommandBuffer buffer{ device, queueFamilyIndex, renderpass,
		                           framebuffer, viewPort, scissor };
	buffer.create();
	secondaryCmdBuffers.push_back(buffer);
	return secondaryCmdBuffers.back();
}

void CommandBuffer::createSecondary(uint32_t count)
{
	secondaryCmdBuffers.resize(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		secondaryCmdBuffers[i] = { device, queueFamilyIndex, renderpass,
			                       framebuffer, viewPort, scissor };
		secondaryCmdBuffers[i].create();
	}
}

// drawing functions
void CommandBuffer::drawIndexed(uint32_t indexCount)
{
	cmdBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void CommandBuffer::drawQuad()
{
	cmdBuffer.draw(3, 1, 0, 0);
}

// secondary command buffer functions ===========================
SecondaryCommandBuffer::SecondaryCommandBuffer()
{
}

SecondaryCommandBuffer::SecondaryCommandBuffer(vk::Device dev, uint32_t index,
                                               vk::RenderPass &rpass, vk::Framebuffer &fbuffer,
                                               vk::Viewport &view, vk::Rect2D &_scissor)
{
	init(dev, index, rpass, fbuffer, view, _scissor);
}

SecondaryCommandBuffer::~SecondaryCommandBuffer()
{
}

void SecondaryCommandBuffer::init(vk::Device dev, uint32_t index, vk::RenderPass &rpass,
                                  vk::Framebuffer &fbuffer, vk::Viewport &view,
                                  vk::Rect2D &_scissor)
{
	device = dev;
	queueFamilyIndex = index;
	framebuffer = fbuffer;
	renderpass = rpass;
	viewPort = view;
	scissor = _scissor;
}

void SecondaryCommandBuffer::end()
{
	cmdBuffer.end();
}

void SecondaryCommandBuffer::create()
{
	vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
	                                     queueFamilyIndex);

	device.createCommandPool(&createInfo, nullptr, &cmdPool);

	// create secondary cmd buffers
	vk::CommandBufferAllocateInfo allocInfo(cmdPool, vk::CommandBufferLevel::eSecondary, 1);
	VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmdBuffer));
}

void SecondaryCommandBuffer::begin()
{
	vk::CommandBufferInheritanceInfo inheritanceInfo(renderpass, 0, framebuffer, VK_FALSE,
	                                                 (vk::QueryControlFlagBits)0,
	                                                 (vk::QueryPipelineStatisticFlagBits)0);

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue,
	                                     &inheritanceInfo);
	VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
}

void SecondaryCommandBuffer::bindPipeline(Pipeline &pipeline)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(pipeline.getPipelineType());
	cmdBuffer.bindPipeline(bindPoint, pipeline.get());
}

void SecondaryCommandBuffer::bindDescriptors(PipelineLayout &pipelineLayout,
                                             DescriptorSet &descriptorSet, PipelineType type)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(bindPoint, pipelineLayout.get(), 0,
	                             static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
}

void SecondaryCommandBuffer::bindDynamicDescriptors(PipelineLayout &pipelineLayout,
                                                    DescriptorSet &descriptorSet, PipelineType type,
                                                    std::vector<uint32_t> &dynamicOffsets)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(
	    bindPoint, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(),
	    static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
}

void SecondaryCommandBuffer::bindDynamicDescriptors(PipelineLayout &pipelineLayout,
                                                    DescriptorSet &descriptorSet, PipelineType type,
                                                    uint32_t &dynamicOffset)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	std::vector<vk::DescriptorSet> sets = descriptorSet.get();
	cmdBuffer.bindDescriptorSets(bindPoint, pipelineLayout.get(), 0,
	                             static_cast<uint32_t>(sets.size()), sets.data(), 1,
	                             &dynamicOffset);
}

void SecondaryCommandBuffer::bindDynamicDescriptors(PipelineLayout &pipelineLayout,
                                                    std::vector<vk::DescriptorSet> &descriptorSet,
                                                    PipelineType type,
                                                    std::vector<uint32_t> &dynamicOffsets)
{
	vk::PipelineBindPoint bindPoint = createBindPoint(type);
	cmdBuffer.bindDescriptorSets(
	    bindPoint, pipelineLayout.get(), 0, static_cast<uint32_t>(descriptorSet.size()),
	    descriptorSet.data(), static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
}

void SecondaryCommandBuffer::bindPushBlock(PipelineLayout &pipelineLayout,
                                           vk::ShaderStageFlags stage, uint32_t size, void *data)
{
	cmdBuffer.pushConstants(pipelineLayout.get(), stage, 0, size, data);
}

void SecondaryCommandBuffer::bindVertexBuffer(vk::Buffer &buffer, vk::DeviceSize offset)
{
	cmdBuffer.bindVertexBuffers(0, 1, &buffer, &offset);
}

void SecondaryCommandBuffer::bindIndexBuffer(vk::Buffer &buffer, uint32_t offset)
{
	cmdBuffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
}

void SecondaryCommandBuffer::setViewport()
{
	cmdBuffer.setViewport(0, 1, &viewPort);
}

void SecondaryCommandBuffer::setScissor()
{
	cmdBuffer.setScissor(0, 1, &scissor);
}

void SecondaryCommandBuffer::setDepthBias(float biasConstant, float biasClamp, float biasSlope)
{
	cmdBuffer.setDepthBias(biasConstant, biasClamp, biasSlope);
}

void SecondaryCommandBuffer::drawIndexed(uint32_t indexCount)
{
	cmdBuffer.drawIndexed(indexCount, 1, 0, 0, 0);
}

void SecondaryCommandBuffer::drawIndexed(const uint32_t indexCount, const uint32_t indexOffset)
{
	cmdBuffer.drawIndexed(indexCount, 1, indexOffset, 0, 0);
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

} // namespace VulkanAPI
