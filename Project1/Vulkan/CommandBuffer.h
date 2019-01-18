#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/MemoryAllocator.h"

namespace VulkanAPI
{

	// we don't need to always have all the functionality of the cmdBuffer class. So these are utility functions for creating single use buffers for copying, etc.
	namespace Util
	{
		vk::CommandBuffer beginSingleCmdBuffer(const vk::CommandPool cmdPool, vk::Device device);
		void submitToQueue(const vk::CommandBuffer cmdBuffer, const vk::Queue queue, const vk::CommandPool cmdPool, vk::Device device);
	}

	// forward decleartions
	class Pipeline;
	class PipelineLayout;
	class Buffer;
	class DescriptorSet;
	enum class PipelineType;

	// index into the secondary command buffer container
	using SecondaryHandle = uint32_t;

	class CommandBuffer
	{

	public:

		struct ThreadedInfo
		{
			vk::CommandPool pool;

		};

		CommandBuffer(vk::Device device);
		~CommandBuffer();

		void create_primary();

		SecondaryHandle begin_secondary();
		void begin_renderpass(vk::RenderPassBeginInfo& begin_info);

		// primary binding functions
		void bind_pipeline(Pipeline& pipeline);
		void bind_vertex_buffer(MemorySegment& vertex_buffer);
		void bind_index_buffer(MemorySegment& index_buffer);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, uint32_t offset_count, uint32_t* offsets, PipelineType type);
		void bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data);

		// secondary binding functions
		void secondary_bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type, SecondaryHandle handle);
		void secondary_bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data, SecondaryHandle handle);

		// drawing functions
		void draw_indexed_quad();

		void create_quad_data();

	private:

		vk::Device device;
		vk::CommandBuffer cmd_buffer;
		vk::Viewport view_port;
		vk::Rect2D scissor;

		// store locally the framebuffer and renderpass associated with this cmd buffer
		vk::RenderPass renderpass;
		vk::Framebuffer framebuffer;

		// if were using, then store all secondary command buffers for dispatching on each thread
		std::vector<vk::CommandBuffer> secondary_cmd_buffers;

		// full screen quad buffers
		struct QuadBuffers
		{
			VulkanAPI::MemorySegment vertex_buffer;
			VulkanAPI::MemorySegment index_buffer;
		} quad_buffers;
	};

}

