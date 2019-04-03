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

	class SecondaryCommandBuffer
	{
        
	public: 

		SecondaryCommandBuffer();
		SecondaryCommandBuffer(vk::Device dev, uint64_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor);
		~SecondaryCommandBuffer();

		void init(vk::Device dev, uint64_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor);
		void create();
		void begin();
		void end();

		// secondary binding functions
		void bind_pipeline(Pipeline& pipeline);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type);
		void bind_dynamic_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type, std::vector<uint32_t>& dynamic_offsets);
		void bind_dynamic_descriptors(PipelineLayout& pl_layout, std::vector < vk::DescriptorSet>& descr_set, PipelineType type, std::vector<uint32_t>& dynamic_offsets);
		void bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data);
		void bind_vertex_buffer(MemorySegment& vertex_buffer, vk::DeviceSize offsets);
		void bind_vertex_buffer(vk::Buffer& buffer, vk::DeviceSize offset);
		void bind_index_buffer(MemorySegment& index_buffer, uint32_t offset);
		void bind_index_buffer(vk::Buffer& buffer, uint32_t offset);

		void set_viewport();
		void set_scissor();

		void draw_indexed(uint32_t index_count);

		// helper funcs
		vk::CommandBuffer& get()
		{
			return cmd_buffer;
		}

		vk::CommandPool& get_pool()
		{
			return cmd_pool;
		}

	private:

		vk::Device device;
		uint64_t queue_family_index;
		
		vk::Viewport view_port;
		vk::Rect2D scissor;

		// store locally the framebuffer and renderpass associated with this cmd buffer
		vk::RenderPass renderpass;
		vk::Framebuffer framebuffer;

		// secondary cmd buffer
		vk::CommandBuffer cmd_buffer;

		// secondary cmd pool for this buffer
		vk::CommandPool cmd_pool;

	};

	class CommandBuffer
	{

	public:
        
        enum class UsageType
        {
            Single,
            Multi
        };
        
		CommandBuffer();
		CommandBuffer(vk::Device dev, uint64_t q_family_index, UsageType type);
		~CommandBuffer();

		void init(vk::Device dev, uint64_t q_family_index, UsageType type);
		void create_primary();

		void begin_renderpass(vk::RenderPassBeginInfo& begin_info, bool use_secondary = false);
		void begin_renderpass(vk::RenderPassBeginInfo& begin_info, vk::Viewport& view_port);
		void end_pass();
		void end();

		// viewport, scissors, etc.
		void set_viewport();
		void set_scissor();

		// primary binding functions
		void bind_pipeline(Pipeline& pipeline);
		void bind_vertex_buffer(vk::Buffer& buffer, vk::DeviceSize offset);
		void bind_index_buffer(MemorySegment& index_buffer);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, PipelineType type);
		void bind_descriptors(PipelineLayout& pl_layout, DescriptorSet& descr_set, uint32_t offset_count, uint32_t* offsets, PipelineType type);
		void bind_push_block(PipelineLayout& pl_layout, vk::ShaderStageFlags stage, uint32_t size, void* data);

		// secondary buffers
		SecondaryCommandBuffer& create_secondary();
		void create_secondary(uint32_t count);

		// functions to execute all secondary buffers associated with this cmd buffer
		void execute_secondary_commands();
		void execute_secondary_commands(uint32_t buffer_count);

		// drawing functions
		void draw_indexed(uint32_t index_count);
		void draw_quad();

		// command pool
		void create_cmd_pool();

		// helper funcs
		vk::CommandBuffer& get()
		{
			return cmd_buffer;
		}

		vk::CommandPool& get_pool()
		{
			return cmd_pool;
		}

		SecondaryCommandBuffer get_secondary(uint32_t index)
		{
			assert(index < secondary_cmd_buffers.size());
			return secondary_cmd_buffers[index];
		}

	private:

		vk::Device device;
		uint64_t queue_family_index;
		
		// the type of cmd buffer, single or multi use, will decide the types of cmd pool, etc. to use
		UsageType usage_type;

		// primary command buffer
		vk::CommandBuffer cmd_buffer;
		
		vk::Viewport view_port;
		vk::Rect2D scissor;

		// store locally the framebuffer and renderpass associated with this cmd buffer
		vk::RenderPass renderpass;
		vk::Framebuffer framebuffer;

		// if were using, then store all secondary command buffers for dispatching on each thread
		std::vector<SecondaryCommandBuffer> secondary_cmd_buffers;

		// primary cmd pool for this buffer
		vk::CommandPool cmd_pool;

	};

}

