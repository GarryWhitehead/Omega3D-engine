#include "CommandBuffer.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Descriptors.h"
#include "OEMaths/OEMaths.h"

namespace VulkanAPI
{

	namespace Util
	{
		vk::CommandBuffer beginSingleCmdBuffer(const vk::CommandPool cmdPool, vk::Device device)
		{
			vk::CommandBufferAllocateInfo allocInfo(cmdPool, vk::CommandBufferLevel::ePrimary, 1);

			vk::CommandBuffer cmdBuffer;
			VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmdBuffer));

			vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, 0);
			VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));

			return cmdBuffer;
		}

		void submitToQueue(const vk::CommandBuffer cmdBuffer, const vk::Queue queue, const vk::CommandPool cmdPool, vk::Device device)
		{
			cmdBuffer.end();

			vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &cmdBuffer, 0, nullptr);
			VK_CHECK_RESULT(queue.submit(1, &submitInfo, {}));
			queue.waitIdle();

			device.freeCommandBuffers(cmdPool, 1, &cmdBuffer);
		}
	}

	vk::PipelineBindPoint create_bind_point(PipelineType type)
	{
		vk::PipelineBindPoint bind_point;

		switch (type) {
		case PipelineType::Graphics:
			bind_point = vk::PipelineBindPoint::eGraphics;
			break;
		case PipelineType::Compute:
			bind_point = vk::PipelineBindPoint::eCompute;
			break;
		}
		return bind_point;
	}

	CommandBuffer::CommandBuffer()
	{
	}

	CommandBuffer::CommandBuffer(vk::Device dev, uint32_t q_family_index) :
		device(dev),
		queue_family_index(q_family_index),
		usage_type(UsageType::Single)
	{
		// create a cmd pool for this buffer
		create_cmd_pool();
	}

	CommandBuffer::CommandBuffer(vk::Device dev, uint32_t q_family_index, UsageType type) :
		device(dev),
		queue_family_index(q_family_index),
		usage_type(type)
	{
		// create a cmd pool for this buffer
		create_cmd_pool();
	}


	CommandBuffer::~CommandBuffer()
	{
	}

	void CommandBuffer::init(vk::Device dev, uint32_t q_family_index)
	{
		device = dev;
		queue_family_index = q_family_index;
		usage_type = UsageType::Single;

		// create a cmd pool for this buffer
		create_cmd_pool();
	}

	void CommandBuffer::createPrimary()
	{
		vk::CommandBufferAllocateInfo allocInfo(cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

		VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmdBuffer));

		vk::CommandBufferUsageFlags usage_flags;
		if (usage_type == UsageType::Single) {
			usage_flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
		}
		else {
			usage_flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
		}

		vk::CommandBufferBeginInfo beginInfo(usage_flags, 0);
		VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
	}

	void CommandBuffer::beginRenderpass(vk::RenderPassBeginInfo& beginInfo, bool use_secondary)
	{
		// stor the renderpass and framebuffer locally
		renderpass = beginInfo.renderPass;
		framebuffer = beginInfo.framebuffer;

		assert(renderpass);
		assert(framebuffer);

		// set viewport and scissor using values from renderpass. Shoule be overridable too
		view_port = vk::Viewport((float)beginInfo.renderArea.offset.x, (float)beginInfo.renderArea.offset.y,
			(float)beginInfo.renderArea.extent.width, (float)beginInfo.renderArea.extent.height,
			0.0f, 1.0f);

		scissor = vk::Rect2D({ { beginInfo.renderArea.offset.x, beginInfo.renderArea.offset.y },
			{ beginInfo.renderArea.extent.width, beginInfo.renderArea.extent.height } });

		if (!use_secondary) {
			cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eInline);
		}
		else {
			cmdBuffer.beginRenderPass(&beginInfo, vk::SubpassContents::eSecondaryCommandBuffers);
		}
	}

	void CommandBuffer::beginRenderpass(vk::RenderPassBeginInfo& beginInfo, vk::Viewport& view_port)
	{
		// stor the renderpass and framebuffer locally
		renderpass = beginInfo.renderPass;
		framebuffer = beginInfo.framebuffer;

		assert(renderpass);
		assert(framebuffer);

		// use custom defined viewport
		vk::Rect2D scissor({ { static_cast<int32_t>(view_port.x), static_cast<int32_t>(view_port.y) },
			{ static_cast<uint32_t>(view_port.width), static_cast<uint32_t>(view_port.height) } });

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
		cmdBuffer.setViewport(0, 1, &view_port);
	}

	void CommandBuffer::setScissor()
	{
		cmdBuffer.setScissor(0, 1, &scissor);
	}

	void CommandBuffer::bindPipeline(Pipeline& pipeline)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(pipeline.get_pipeline_type());
		cmdBuffer.bindPipeline(bind_point, pipeline.get());
	}

	void CommandBuffer::bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descriptorSet.get();
		cmdBuffer.bindDescriptorSets(bind_point, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
	}

	void CommandBuffer::bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, uint32_t offset_count, uint32_t* offsets, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descriptorSet.get();
		cmdBuffer.bindDescriptorSets(bind_point, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(), offset_count, offsets);
	}

	void CommandBuffer::bindPushBlock(PipelineLayout& pipelineLayout, vk::ShaderStageFlags stage, uint32_t size, void* data)
	{
		cmdBuffer.pushConstants(pipelineLayout.get(), stage, 0, size, data);
	}

	void CommandBuffer::setDepthBias(float biasConstant, float biasClamp, float biasSlope)
	{
		cmdBuffer.setDepthBias(biasConstant, biasClamp, biasSlope);
	}

	void CommandBuffer::executeSecondaryCommands()
	{
		assert(!secondary_cmdBuffers.empty());

		// trasnfer all the secondary cmd buffers into a container for execution
		std::vector<vk::CommandBuffer> secondaryCmdBuffers(secondary_cmdBuffers.size());
		for (uint32_t i = 0; i < secondary_cmdBuffers.size(); ++i) {
			secondaryCmdBuffers[i] = secondary_cmdBuffers[i].get();
		}

		cmdBuffer.executeCommands(static_cast<uint32_t>(secondaryCmdBuffers.size()), secondaryCmdBuffers.data());
	}

	void CommandBuffer::executeSecondaryCommands(uint32_t count)
	{
		assert(!secondary_cmdBuffers.empty());

		// trasnfer the specified amount of secondary cmd buffers into a container for execution
		std::vector<vk::CommandBuffer> secondaryCmdBuffers(count);
		for (uint32_t i = 0; i < count; ++i) {
			secondaryCmdBuffers[i] = secondary_cmdBuffers[i].get();
		}

		cmdBuffer.executeCommands(count, secondaryCmdBuffers.data());
	}

	SecondaryCommandBuffer& CommandBuffer::createSecondary()
	{
		SecondaryCommandBuffer buffer{ device, queue_family_index, renderpass, framebuffer, view_port, scissor };
		buffer.create();
		secondary_cmdBuffers.push_back(buffer);
		return secondary_cmdBuffers.back();
	}

	void CommandBuffer::createSecondary(uint32_t count)
	{
		secondary_cmdBuffers.resize(count);
		for (uint32_t i = 0; i < count; ++i) {
			secondary_cmdBuffers[i] = {device, queue_family_index, renderpass, framebuffer, view_port, scissor};
			secondary_cmdBuffers[i].create();
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

	SecondaryCommandBuffer::SecondaryCommandBuffer(vk::Device dev, uint32_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor)
	{
		init(dev, q_family_index, rpass, fbuffer, view, _scissor);
	}

	SecondaryCommandBuffer::~SecondaryCommandBuffer()
	{
	}

	void SecondaryCommandBuffer::init(vk::Device dev, uint32_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor)
	{
		device = dev;
		queue_family_index = q_family_index;
		framebuffer = fbuffer;
		renderpass = rpass;
		view_port = view;
		scissor = _scissor;
	}

	void SecondaryCommandBuffer::end()
	{
		cmdBuffer.end();
	}

	void SecondaryCommandBuffer::create()
	{
		vk::CommandPoolCreateInfo create_info(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				queue_family_index);

		device.createCommandPool(&create_info, nullptr, &cmd_pool);

		// create secondary cmd buffers
		vk::CommandBufferAllocateInfo allocInfo(cmd_pool, vk::CommandBufferLevel::eSecondary, 1);
		VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmdBuffer));
	}

	void SecondaryCommandBuffer::begin()
	{
		vk::CommandBufferInheritanceInfo inheritance_info(
			renderpass, 0,
			framebuffer, VK_FALSE,
			(vk::QueryControlFlagBits)0, 
			(vk::QueryPipelineStatisticFlagBits)0);
	
		vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance_info);
		VK_CHECK_RESULT(cmdBuffer.begin(&beginInfo));
	}

	void SecondaryCommandBuffer::bindPipeline(Pipeline& pipeline)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(pipeline.get_pipeline_type());
		cmdBuffer.bindPipeline(bind_point, pipeline.get());
	}

	void SecondaryCommandBuffer::bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descriptorSet.get();
		cmdBuffer.bindDescriptorSets(bind_point, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 0, nullptr);
	}

	void SecondaryCommandBuffer::bindDynamicDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type, std::vector<uint32_t>& dynamicOffsets)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descriptorSet.get();
		cmdBuffer.bindDescriptorSets(bind_point, pipelineLayout.get(), 0, static_cast<uint32_t>(sets.size()), sets.data(), 
			static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
	}

	void SecondaryCommandBuffer::bindDynamicDescriptors(PipelineLayout& pipelineLayout, std::vector<vk::DescriptorSet>& descriptorSet, PipelineType type, std::vector<uint32_t>& dynamicOffsets)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		cmdBuffer.bindDescriptorSets(bind_point, pipelineLayout.get(), 0, static_cast<uint32_t>(descriptorSet.size()), 
			descriptorSet.data(), static_cast<uint32_t>(dynamicOffsets.size()), dynamicOffsets.data());
	}

	void SecondaryCommandBuffer::bindPushBlock(PipelineLayout& pipelineLayout, vk::ShaderStageFlags stage, uint32_t size, void* data)
	{
		cmdBuffer.pushConstants(pipelineLayout.get(), stage, 0, size, data);
	}

	void SecondaryCommandBuffer::bindVertexBuffer(vk::Buffer& buffer, vk::DeviceSize offset)
	{
		cmdBuffer.bindVertexBuffers(0, 1, &buffer, &offset);
	}

	void SecondaryCommandBuffer::bindIndexBuffer(vk::Buffer& buffer, uint32_t offset)
	{
		cmdBuffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
	}

	void SecondaryCommandBuffer::setViewport()
	{
		cmdBuffer.setViewport(0, 1, &view_port);
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

	// command pool functions =====================================================================

	void CommandBuffer::create_cmd_pool()
	{
		vk::CommandPoolCreateInfo create_info(
			usage_type == UsageType::Single ? vk::CommandPoolCreateFlagBits::eTransient : vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			queue_family_index);

		device.createCommandPool(&create_info, nullptr, &cmd_pool);
	}
}
