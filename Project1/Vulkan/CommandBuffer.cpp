#include "CommandBuffer.h"
#include "Vulkan/Pipeline.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/Vulkan_Global.h"
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

	void CommandBuffer::bind_vertex_buffer(MemorySegment& vertex_buffer)
	{
		VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;

		vk::DeviceSize offsets[1] = { quad_buffers.vertex_buffer.get_offset() };
		cmd_buffer.bindVertexBuffers(0, 1, &mem_alloc.get_memory_buffer(vertex_buffer.get_id()), offsets);
	}

	void CommandBuffer::bind_index_buffer(MemorySegment& index_buffer)
	{
		VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		cmd_buffer.bindIndexBuffer(mem_alloc.get_memory_buffer(index_buffer.get_id()), index_buffer.get_offset(), vk::IndexType::eUint32);
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
		// bind quad vertices and indices
		VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		vk::Buffer vert_buffer = mem_alloc.get_memory_buffer(quad_buffers.vertex_buffer.get_id());
		vk::Buffer index_buffer = mem_alloc.get_memory_buffer(quad_buffers.index_buffer.get_id());

		vk::DeviceSize offsets[1] = { quad_buffers.vertex_buffer.get_offset() };
		cmd_buffer.bindVertexBuffers(0, 1, &vert_buffer, offsets);
		cmd_buffer.bindIndexBuffer(index_buffer, quad_buffers.index_buffer.get_offset(), vk::IndexType::eUint32);	// making an assumption here that this is unit32 - let's ensure this is the case
		cmd_buffer.drawIndexed(6, 1, 0, 0, 0);
	}

	void CommandBuffer::create_quad_data()
	{
		struct Vertex
		{
			OEMaths::vec3f position;
			OEMaths::vec2f uv;
			OEMaths::vec3f normal;
		};

		// vertices
		std::vector<Vertex> vertices = {
		{ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f }},
		{ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 0.0f }},
		{ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }},
		{ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f }}
		};

		// prepare indices
		std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
		for (uint32_t i = 0; i < 3; ++i)
		{
			uint32_t values[6] = { 0,1,2, 2,3,0 };
			for (auto index : values)
			{
				indices.push_back(i * 4 + index);
			}
		}

		VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;

		// map vertices
		quad_buffers.vertex_buffer = mem_alloc.allocate(MemoryUsage::VK_BUFFER_STATIC, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(Vertex) * vertices.size());
		mem_alloc.mapDataToSegment(quad_buffers.vertex_buffer, vertices.data(), vertices.size());

		// map indices
		quad_buffers.index_buffer = mem_alloc.allocate(MemoryUsage::VK_BUFFER_STATIC, vk::BufferUsageFlagBits::eIndexBuffer, sizeof(uint32_t) * indices.size());
		mem_alloc.mapDataToSegment(quad_buffers.index_buffer, indices.data(), indices.size());
	}
}
