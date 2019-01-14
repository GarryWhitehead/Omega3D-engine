#include "CommandBuffer.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Buffer.h"

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
			VK_CHECK_RESULT(queue.submit(1, &submitInfo, VK_NULL_HANDLE));
			queue.waitIdle();

			device.freeCommandBuffers(cmdPool, 1, &cmdBuffer);
		}
	}

	static vk::PipelineBindPoint create_bind_point(PipelineType type)
	{
		switch (type) {
		case PipelineType::Graphics:
			return vk::PipelineBindPoint::eGraphics;
			break;
		case PipelineType::Compute:
			return vk::PipelineBindPoint::eCompute;
			break;
		}
	}

	CommandBuffer::CommandBuffer(vk::Device device)
	{
	}


	CommandBuffer::~CommandBuffer()
	{
	}

	void CommandBuffer::begin_renderpass(vk::RenderPassBeginInfo& begin_info)
	{
		// set viewport and scissor using values from renderpass. Shoule be overridable too
		vk::Viewport view_port(begin_info.renderArea.offset.x, begin_info.renderArea.offset.y,
			begin_info.renderArea.extent.width, begin_info.renderArea.extent.height,
			0.0f, 1.0f);

		vk::Rect2D scissor({ { begin_info.renderArea.offset.x, begin_info.renderArea.offset.y },
			{ begin_info.renderArea.extent.width, begin_info.renderArea.extent.height } });

		cmd_buffer.beginRenderPass(&begin_info, vk::SubpassContents::eInline);
	}

	void CommandBuffer::bind_pipeline(Pipeline& pipeline)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(pipeline.get_pipeline_type());
		cmd_buffer.bindPipeline(bind_point, pipeline.get());
	}

	void CommandBuffer::bind_vertex_buffer(Buffer& vertex_buffer)
	{

	}

	void CommandBuffer::bind_index_buffer(Buffer& index_buffer)
	{

	}

	void CommandBuffer::bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, 1, &descr_set.get(), 0, nullptr);
	}

	void CommandBuffer::bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, uint32_t offset_count, uint32_t* offsets, PipelineType type)
	{
		vk::PipelineBindPoint bind_point = create_bind_point(type);
		cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, 1, &descr_set.get(), offset_count, offsets);
	}

	// drawing functions ========
	void CommandBuffer::draw_indexed_quad()
	{

	}
}
