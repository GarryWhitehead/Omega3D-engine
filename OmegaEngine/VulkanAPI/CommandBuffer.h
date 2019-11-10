#pragma once

#include "VulkanAPI/Common.h"

#include <cstdint>

namespace VulkanAPI
{

// forward decleartions
class Pipeline;
class PipelineLayout;
struct Buffer;
class DescriptorSet;
enum class PipelineType;
class VkContext;

class CmdBuffer
{

public:
	enum class UsageType
	{
		Single,
		Multi
	};

	CmdBuffer(VkContext& context);
	~CmdBuffer();

	void createPrimary();

	void beginRenderpass(vk::RenderPassBeginInfo& beginInfo, bool useSecondary = false);
	void beginRenderpass(vk::RenderPassBeginInfo& beginInfo, vk::Viewport& viewPort);
	void endRenderpass();
	void end();

	// viewport, scissors, etc.
	void setViewport();
	void setScissor();
	void setViewport(const vk::Viewport& viewPort);
	void setScissor(const vk::Rect2D& scissor);

	// primary binding functions
	void bindPipeline(Pipeline& pipeline);
	void bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, PipelineType type);
	void bindDescriptors(PipelineLayout& pipelineLayout, DescriptorSet& descriptorSet, uint32_t offsetCount,
	                     uint32_t* offsets, PipelineType type);
	void bindPushBlock(PipelineLayout& pipelineLayout, vk::ShaderStageFlags stage, uint32_t size, void* data);
	void bindVertexBuffer(vk::Buffer& buffer, vk::DeviceSize offset);
	void bindIndexBuffer(vk::Buffer& buffer, uint32_t offset);

	// dynamic bindings
	void setDepthBias(float biasConstant, float biasClamp, float biasSlope);

	// secondary buffers
	CmdBuffer& createSecondary();
	void createSecondary(uint32_t count);

	/**
	* @brief Executes all secondary command buffers associated with the primary one
	* Note: Will segfault if secondaries are empty or invalid count
	* @param count If non-zero, the number of buffers to execute. Otherwise if zero, all buffers will be executed
	*/
	void executeSecondaryCommands(size_t count = 0);

	// drawing functions
	void drawIndexed(uint32_t indexCount);
	void drawQuad();

	// command pool
	void createCmdPool();

	// helper funcs
	vk::CommandBuffer& get()
	{
		return cmdBuffer;
	}

	vk::CommandPool& getCmdPool()
	{
		return cmdPool;
	}

	CmdBuffer getSecondary(uint32_t index)
	{
		assert(index < secondarys.size());
		return secondarys[index];
	}

private:
	vk::Device device;
	uint32_t queueFamilyIndex;

	// the type of cmd buffer, single or multi use, will decide the types of cmd pool, etc. to use
	UsageType usageType;

	// primary command buffer
	vk::CommandBuffer cmdBuffer;

	vk::Viewport viewPort;
	vk::Rect2D scissor;

	// store locally the framebuffer and renderpass associated with this cmd buffer
	vk::RenderPass renderpass;
	vk::Framebuffer framebuffer;

	// if were using, then store all secondary command buffers for dispatching on each thread
	std::vector<CmdBuffer> secondarys;

	// primary cmd pool for this buffer
	vk::CommandPool cmdPool;
};

}    // namespace VulkanAPI
