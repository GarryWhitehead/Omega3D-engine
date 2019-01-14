#pragma once
#include "Vulkan/Common.h"

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

	class CommandBuffer
	{

	public:

		CommandBuffer(vk::Device device);
		~CommandBuffer();

		void begin_renderpass(vk::RenderPassBeginInfo& begin_info);

		void bind_pipeline(Pipeline& pipeline);
		void bind_vertex_buffer(Buffer& vertex_buffer);
		void bind_index_buffer(Buffer& index_buffer);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, uint32_t offset_count, uint32_t* offsets, PipelineType type);

		void draw_indexed_quad();

	private:

		vk::Device device;
		vk::CommandBuffer cmd_buffer;
		vk::Viewport view_port;
		vk::Rect2D scissor;
	};

}

