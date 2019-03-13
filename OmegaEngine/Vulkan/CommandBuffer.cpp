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

	CommandBuffer::CommandBuffer(vk::Device dev) :
		device(dev)
	{
		init(dev);
	}


	CommandBuffer::~CommandBuffer()
	{
	}

	void CommandBuffer::init(vk::Device dev)
	{
		device = dev;
		create_cmd_pool();

		// create a cmd pool for this buffer
		create_cmd_pool();
	}

	void CommandBuffer::create_primary(UsageType type)
	{	
		vk::CommandBufferAllocateInfo allocInfo(cmd_pool, vk::CommandBufferLevel::ePrimary, 1);

		VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &cmd_buffer));
        
        vk::CommandBufferUsageFlags usage_flags;
        if (type == UsageType::Single) {
            usage_flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
        }
        else {
            usage_flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
        }
        
		vk::CommandBufferBeginInfo beginInfo(usage_flags, 0);
		VK_CHECK_RESULT(cmd_buffer.begin(&beginInfo));
	}

	void CommandBuffer::create_secondary(uint32_t count, bool reset)
	{
		if (reset) {
			secondary_cmd_buffers.clear();
			secondary_cmd_pools.clear();
		}

		// create pool for each secondary
		for (uint32_t i = 0; i < count; ++i) {

			vk::CommandPool pool;
			vk::CommandPoolCreateInfo create_info(
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				queue_family_index);

			device.createCommandPool(&create_info, nullptr, &pool);
			secondary_cmd_pools.push_back(pool);

			// create secondary cmd buffers
			vk::CommandBuffer sec_cmd_buffer;
			vk::CommandBufferAllocateInfo allocInfo(pool, vk::CommandBufferLevel::eSecondary, 1);
			VK_CHECK_RESULT(device.allocateCommandBuffers(&allocInfo, &sec_cmd_buffer));
			secondary_cmd_buffers.push_back(sec_cmd_buffer);
		}
	}

	void CommandBuffer::begin_secondary(uint32_t index)
	{
		vk::CommandBufferInheritanceInfo inheritance_info(
			renderpass, 0,
			framebuffer, VK_FALSE,
			(vk::QueryControlFlagBits)0, 
			(vk::QueryPipelineStatisticFlagBits)0);

		vk::CommandBufferAllocateInfo allocInfo(cmd_pool, vk::CommandBufferLevel::eSecondary, 1);
		
		vk::CommandBufferBeginInfo begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, &inheritance_info);
		VK_CHECK_RESULT(secondary_cmd_buffers[index].begin(&begin_info));
	}

	void CommandBuffer::begin_renderpass(vk::RenderPassBeginInfo& begin_info, bool use_secondary)
	{
		// stor the renderpass and framebuffer locally
		renderpass = begin_info.renderPass;
		framebuffer = begin_info.framebuffer;

		assert(renderpass);
		assert(framebuffer);
		
		// set viewport and scissor using values from renderpass. Shoule be overridable too
		vk::Viewport view_port(begin_info.renderArea.offset.x, begin_info.renderArea.offset.y,
			begin_info.renderArea.extent.width, begin_info.renderArea.extent.height,
			0.0f, 1.0f);

		vk::Rect2D scissor({ { begin_info.renderArea.offset.x, begin_info.renderArea.offset.y },
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

	// secondary command buffer functions ===========================
	void CommandBuffer::bind_secondary_pipeline(Pipeline& pipeline, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		vk::PipelineBindPoint bind_point = create_bind_point(pipeline.get_pipeline_type());
		sec_cmd_buffer.bindPipeline(bind_point, pipeline.get());
	}

	void CommandBuffer::secondary_bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descr_set.get();
		sec_cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, sets.size(), sets.data(), 0, nullptr);
	}

	void CommandBuffer::secondary_bind_dynamic_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type, std::vector<uint32_t>& dynamic_offsets, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		assert(!dynamic_offsets.empty());
		
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		vk::PipelineBindPoint bind_point = create_bind_point(type);
		std::vector<vk::DescriptorSet> sets = descr_set.get();
		sec_cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, sets.size(), sets.data(), static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
	}

	void CommandBuffer::secondary_bind_dynamic_descriptors(PipelineLayout& pl_layout, std::vector<vk::DescriptorSet>& descr_set, PipelineType type, std::vector<uint32_t>& dynamic_offsets, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		assert(!dynamic_offsets.empty());

		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		vk::PipelineBindPoint bind_point = create_bind_point(type);
		sec_cmd_buffer.bindDescriptorSets(bind_point, pl_layout.get(), 0, descr_set.size(), descr_set.data(), static_cast<uint32_t>(dynamic_offsets.size()), dynamic_offsets.data());
	}

	void CommandBuffer::secondary_bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		sec_cmd_buffer.pushConstants(pl_layout.get(), stage, 0, size, data);
	}

	void CommandBuffer::secondary_bind_vertex_buffer(MemorySegment& vertex_buffer, vk::DeviceSize offset, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		sec_cmd_buffer.bindVertexBuffers(0, 1, &mem_alloc.get_memory_buffer(vertex_buffer.get_id()), &offset);
	}

	void CommandBuffer::secondary_bind_vertex_buffer(vk::Buffer& buffer, vk::DeviceSize offset, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];
		sec_cmd_buffer.bindVertexBuffers(0, 1, &buffer, &offset);
	}

	void CommandBuffer::secondary_bind_index_buffer(MemorySegment& index_buffer, uint32_t offset, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];

		VulkanAPI::MemoryAllocator& mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		sec_cmd_buffer.bindIndexBuffer(mem_alloc.get_memory_buffer(index_buffer.get_id()), offset, vk::IndexType::eUint32);
	}

	void CommandBuffer::secondary_bind_index_buffer(vk::Buffer& buffer, uint32_t offset, SecondaryHandle handle)
	{
		assert(!secondary_cmd_buffers.empty() && secondary_cmd_buffers.size() > handle);
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];
		sec_cmd_buffer.bindIndexBuffer(buffer, offset, vk::IndexType::eUint32);
	}

	void CommandBuffer::secondary_execute_commands()
	{
		assert(!secondary_cmd_buffers.empty());
		cmd_buffer.executeCommands(secondary_cmd_buffers.size(), secondary_cmd_buffers.data());
	}

	
	// drawing functions ========
	void CommandBuffer::draw_indexed(uint32_t index_count)
	{
		cmd_buffer.drawIndexed(index_count, 1, 0, 0, 0);
	}

	void CommandBuffer::secondary_draw_indexed(uint32_t index_count, SecondaryHandle handle)
	{
		vk::CommandBuffer sec_cmd_buffer = secondary_cmd_buffers[handle];
		sec_cmd_buffer.drawIndexed(index_count, 1, 0, 0, 0);
	}

	void CommandBuffer::draw_quad()
	{
		cmd_buffer.draw(3, 1, 0, 0);
	}

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

	// command pool functions =====================================================================

	void CommandBuffer::create_cmd_pool()
	{
		vk::CommandPoolCreateInfo create_info(
			vk::CommandPoolCreateFlagBits::eTransient,
			queue_family_index);

		device.createCommandPool(&create_info, nullptr, &cmd_pool);
	}
}
