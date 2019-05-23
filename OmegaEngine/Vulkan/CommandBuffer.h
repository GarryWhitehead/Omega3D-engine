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
	struct Buffer;
	class DescriptorSet;
	enum class PipelineType;

	class SecondaryCommandBuffer
	{
        
	public: 

		SecondaryCommandBuffer();
		SecondaryCommandBuffer(vk::Device dev, uint32_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor);
		~SecondaryCommandBuffer();

		void init(vk::Device dev, uint32_t q_family_index, vk::RenderPass& rpass, vk::Framebuffer& fbuffer, vk::Viewport& view, vk::Rect2D& _scissor);
		void create();
		void begin();
		void end();

		// secondary binding functions
		void bindPipeline(Pipeline& pipeline);
		void bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type);
		void bindDynamicDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type, std::vector<uint32_t>& dynamicOffsets);
		void bindDynamicDescriptors(PipelineLayout& pipelineLayout, std::vector < vk::DescriptorSet>& descriptorSet, PipelineType type, std::vector<uint32_t>& dynamicOffsets);
		void bindPushBlock(PipelineLayout& pipelineLayout, vk::ShaderStageFlags stage, uint32_t size, void* data);
		void bindVertexBuffer(vk::Buffer& buffer, vk::DeviceSize offset);
		void bindIndexBuffer(vk::Buffer& buffer, uint32_t offset);

		void setViewport();
		void setScissor();
		void setDepthBias(float biasConstant, float biasClamp, float biasSlope);

		void drawIndexed(uint32_t indexCount);

		// helper funcs
		vk::CommandBuffer& get()
		{
			return cmdBuffer;
		}

		vk::CommandPool& get_pool()
		{
			return cmd_pool;
		}

	private:

		vk::Device device;
		uint32_t queue_family_index;
		
		vk::Viewport view_port;
		vk::Rect2D scissor;

		// store locally the framebuffer and renderpass associated with this cmd buffer
		vk::RenderPass renderpass;
		vk::Framebuffer framebuffer;

		// secondary cmd buffer
		vk::CommandBuffer cmdBuffer;

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
		CommandBuffer(vk::Device dev, uint32_t q_family_index);
		CommandBuffer(vk::Device dev, uint32_t q_family_index, UsageType type);
		~CommandBuffer();

		void init(vk::Device dev, uint32_t q_family_index);
		void createPrimary();

		void beginRenderpass(vk::RenderPassBeginInfo& beginInfo, bool use_secondary = false);
		void beginRenderpass(vk::RenderPassBeginInfo& beginInfo, vk::Viewport& view_port);
		void endRenderpass();
		void end();

		// viewport, scissors, etc.
		void setViewport();
		void setScissor();

		// primary binding functions
		void bindPipeline(Pipeline& pipeline);
		void bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type);
		void bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, uint32_t offset_count, uint32_t* offsets, PipelineType type);
		void bindPushBlock(PipelineLayout& pipelineLayout, vk::ShaderStageFlags stage, uint32_t size, void* data);

		// dynamic bindings
		void setDepthBias(float biasConstant, float biasClamp, float biasSlope);

		// secondary buffers
		SecondaryCommandBuffer& createSecondary();
		void createSecondary(uint32_t count);

		// functions to execute all secondary buffers associated with this cmd buffer
		void executeSecondaryCommands();
		void executeSecondaryCommands(uint32_t buffer_count);

		// drawing functions
		void drawIndexed(uint32_t indexCount);
		void drawQuad();

		// command pool
		void create_cmd_pool();

		// helper funcs
		vk::CommandBuffer& get()
		{
			return cmdBuffer;
		}

		vk::CommandPool& get_pool()
		{
			return cmd_pool;
		}

		SecondaryCommandBuffer getSecondary(uint32_t index)
		{
			assert(index < secondary_cmdBuffers.size());
			return secondary_cmdBuffers[index];
		}

	private:

		vk::Device device;
		uint32_t queue_family_index;
		
		// the type of cmd buffer, single or multi use, will decide the types of cmd pool, etc. to use
		UsageType usage_type;

		// primary command buffer
		vk::CommandBuffer cmdBuffer;
		
		vk::Viewport view_port;
		vk::Rect2D scissor;

		// store locally the framebuffer and renderpass associated with this cmd buffer
		vk::RenderPass renderpass;
		vk::Framebuffer framebuffer;

		// if were using, then store all secondary command buffers for dispatching on each thread
		std::vector<SecondaryCommandBuffer> secondary_cmdBuffers;

		// primary cmd pool for this buffer
		vk::CommandPool cmd_pool;

	};

}

