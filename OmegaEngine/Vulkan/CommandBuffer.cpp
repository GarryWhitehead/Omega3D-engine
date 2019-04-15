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

	CommandBuffer::CommandBuffer(vk::Device dev, uint64_t q_family_index, UsageType type) :
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

	void CommandBuffer::init(vk::Device dev, uint64_t q_family_index, UsageType type)
	{
		device = dev;
		queue_family_index = q_family_index;
		usage_type = type;

		// create a cmd pool for this buffer
		create_cmd_pool();
	}

	void CommandBuffer::create_primary()
	{	
		vk::CommandBufferAllocateInfo allocInfo(cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

		VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmd_buffer));
        
        vk::CommandBufferUsageFlags usage_flags;
        if (usage_type == UsageType::Single) {
            usage_flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        }
        else {
            usage_flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
        }
        
		vk::CommandBufferBeginInfo beginInfo(usage_flags, 0);
		VK_CHECK_RESULT(cmd_buffer.begin(&beginInfo));
	}

	void CommandBuffer::begin_renderpass(vk::RenderPassBeginInfo& begin_info, bool use_secondary)
	{
		// stor the renderpass and framebuffer locally
		renderpass = begin_info.renderPass;
		framebuffer = begin_info.framebuffer;

		assert(renderpass);
		assert(framebuffer);
		
		// set viewport and scissor using values from renderpass. Shoule be overridable too
		view_port = vk::Viewport(begin_info.renderArea.offset.x, begin_info.renderArea.offset.y,
			begin_info.renderArea.extent.width, begin_info.renderArea.extent.height,
			0.0f, 1.0f);

		scissor = vk::Rect2D({ { begin_info.renderArea.offset.x, begin_info.renderArea.offset.y },
			{ begin_info.renderArea.extent.width, begin_info.renderArea.extent.height } });

		if (!use_secondary) {
			cmd_buffer.beginRenderPass(&begin_info, vk::SubpassContents::eInline);
		}
		else {
			cmd_buffer.beginRenderPass(&begin_info, vk::SubpassContents::eSecondaryCommandBuffers);
		}
	}

	void CommandBuffer::begin_renderpass(vk::RenderPassBeginInfo& begin_info, vk::Viewport& view_port)
	{
		// stor the renderpass and framebuffer locally
		renderpass = begin_info.renderPass;
		framebuffer = begin_info.framebuffer;

		assert(renderpass);
		assert(framebuffer);

		// use custom defined viewport
		vk::Rect2D scissor({ { static_cast<int32_t>(view_port.x), static_cast<int32_t>(view_port.y) },
			{ static_cast<uint32_t>(view_port.width), static_cast<uint32_t>(view_port.height) } });

		cmd_buffer.beginRenderPass(&begin_info, vk::SubpassContents::eInline);
	}

	void CommandBuffer::end_pass()
	{
		cmd_buffer.endRenderPass();
	}

	void CommandBuffer::end()
	{
		cmd_buffer.end();
	}

	void CommandBuffer::set_viewport()
	{
		cmd_buffer.setViewport(0, 1, &view_port);
	}

	void CommandBuffer::set_scissor()
	{
		cmd_buffer.setScissor(0, 1, &scissor);
	}

	void CommandBuffer::bind_pipeline(Pipeline& pipeline)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(pipeline.get_pipeline_type());
		cmd_buffer.bindPipeline(bind_point, pipeline.get());
	}

	void CommandBuffer::bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descr_set.get();
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, sets.size(), sets.data(), 0, nullptr);
	}

	void CommandBuffer::bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, uint32_t offset_count, uint32_t* offsets, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descr_set.get();
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, sets.size(), sets.data(), offset_count, offsets);
	}

	void CommandBuffer::bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data)
	{
		cmd_buffer.pushConstants(pl_layout.get(), stage, 0, size, data);
	}

	void CommandBuffer::execute_secondary_commands()
	{
		assert(!secondary_cmd_buffers.empty());

		// trasnfer all the secondary cmd buffers into a container for execution
		std::vector<vk::CommandBuffer> sec_cmd_buffers(secondary_cmd_buffers.size());
		for (uint32_t i = 0; i < secondary_cmd_buffers.size(); ++i) {
			sec_cmd_buffers[i] = secondary_cmd_buffers[i].get();
		}

		cmd_buffer.executeCommands(sec_cmd_buffers.size(), sec_cmd_buffers.data());
	}

	void CommandBuffer::execute_secondary_commands(uint32_t count)
	{
		assert(!secondary_cmd_buffers.empty());

		// trasnfer the specified amount of secondary cmd buffers into a container for execution
		std::vector<vk::CommandBuffer> sec_cmd_buffers(count);
		for (uint32_t i = 0; i < count; ++i) {
			sec_cmd_buffers[i] = secondary_cmd_buffers[i].get();
		}

		cmd_buffer.executeCommands(count, sec_cmd_buffers.data());
	}

	SecondaryCommandBuffer& CommandBuffer::create_secondary()
	{
		SecondaryCommandBuffer buffer{ device, queue_family_index, renderpass, framebuffer, view_port, scissor };
		buffer.create();
		secondary_cmd_buffers.push_back(buffer);
		return secondary_cmd_buffers.back();
	}

	void CommandBuffer::create_secondary(uint32_t count)
	{
		secondary_cmd_buffers.resize(count);
		for (uint32_t i = 0; i < count; ++i) {
			secondary_cmd_buffers[i] = {device, queue_family_index, renderpass, framebuffer, view_port, scissor};
			secondary_cmd_buffers[i].create();
		}
	}

	// drawing functions 
	void CommandBuffer::draw_indexed(uint32_t index_count)
	{
		cmd_buffer.drawIndexed(index_count, 1, 0, 0, 0);
	}

	void CommandBuffer::draw_quad()
	{
		cmd_buffer.draw(3, 1, 0, 0);
	}

	// secondary command buffer functions ===========================
	SecondaryCommandBuffer::SecondaryCommandBuffer()
	{
	}

	SecondaryCommandBuffer::SecondaryCommandBuffer(vk::Device dev, uint64_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor)
	{
		init(dev, q_family_index, rpass, fbuffer, view, _scissor);
	}

	SecondaryCommandBuffer::~SecondaryCommandBuffer()
	{
	}

	void SecondaryCommandBuffer::init(vk::Device dev, uint64_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor)
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
		cmd_buffer.end();
	}

	void SecondaryCommandBuffer::create()
	{
		vk::CommandPoolCreateInfo create_info(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				queue_family_index);

		device.createCommandPool(&create_info, nullptr, &cmd_pool);

		// create secondary cmd buffers
		vk::CommandBufferAllocateInfo allocInfo(cmd_pool, vk::CommandBufferLevel::eSecondary, 1);
		VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmd_buffer));
	}

	void SecondaryCommandBuffer::begin()
	{
		vk::CommandBufferInheritanceInfo inheritance_info(
			renderpass, 0,
			framebuffer, VK_FALSE,
			(vk::QueryControlFlagBits)0, 
			(vk::QueryPipelineStatisticFlagBits)0);
	
		vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eRenderPassContinue, &inheritance_info);
		VK_CHECK_RESULT(cmd_buffer.begin(&begin_info));
	}

	void SecondaryCommandBuffer::bind_pipeline(Pipeline& pipeline)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(pipeline.get_pipeline_type());
		cmd_buffer.bindPipeline(bind_point, pipeline.get());
	}

	void SecondaryCommandBuffer::bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descr_set.get();
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, sets.size(), sets.data(), 0, nullptr);
	}

	void SecondaryCommandBuffer::bind_dynamic_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type, std::vector<uint32_t>& dynamic_offsets)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descr_set.get();
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, sets.size(), sets.data(), static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
	}

	void SecondaryCommandBuffer::bind_dynamic_descriptors(PipelineLayout& pl_layout, std::vector<vk::DescriptorSet>& descr_set, PipelineType type, std::vector<uint32_t>& dynamic_offsets)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, descr_set.size(), descr_set.data(), static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
	}

	void SecondaryCommandBuffer::bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data)
	{
		cmd_buffer.pushConstants(pl_layout.get(), stage, 0, size, data);
	}

	void SecondaryCommandBuffer::bind_vertex_buffer(vk::Buffer& buffer, vk::DeviceSize offset)
	{
		cmd_buffer.bindVertexBuffers(0, 1, &buffer, &offset);
	}

	void SecondaryCommandBuffer::bind_index_buffer(vk::Buffer& buffer, uint32_t offset)
	{
		cmd_buffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
	}

	void SecondaryCommandBuffer::set_viewport()
	{
		cmd_buffer.setViewport(0, 1, &view_port);
	}

	void SecondaryCommandBuffer::set_scissor()
	{
		cmd_buffer.setScissor(0, 1, &scissor);
	}

	void SecondaryCommandBuffer::draw_indexed(uint32_t index_count)
	{
		cmd_buffer.drawIndexed(index_count, 1, 0, 0, 0);
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
