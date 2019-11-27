#pragma once

#include "VulkanAPI/Common.h"

#include <cstdint>

namespace VulkanAPI
{

// forward decleartions
class Pipeline;
class PipelineLayout;
class DescriptorSet;
class VkContext;
class CmdBufferManager;

class CmdBuffer
{

public:
	enum class Type
	{
		Primary,
		Secondary
	};

	enum class Usage
	{
		Single,
		Multi
	};

	CmdBuffer(VkContext& context, const Type type, vk::CommandPool* cmdPool = nullptr,
	          CmdBufferManager* cbManager = nullptr);
	~CmdBuffer();

	void prepare();

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
	void executeSecondary(size_t count = 0);
    
    /**
     * @brief Flushes the queue that this cmd buffer is associated with.
     */
    void flush();
    
    /**
     * @brief Submits this cmd buffer to the specified queue
     */
    void submit(vk::Semaphore& waitSemaphore, vk::Semaphore& signalSemaphore,
                       vk::Fence& fence);
    
	// drawing functions
	void drawIndexed(size_t indexCount);
	void drawQuad();

	// helper funcs
	vk::CommandBuffer& get()
	{
		return cmdBuffer;
	}

	CmdBuffer getSecondary(size_t index)
	{
		assert(index < secondarys.size());
		return secondarys[index];
	}

private:
	
	// local vulkan context 
    VkContext& context;
	vk::CommandPool* cmdPool = nullptr;
	CmdBufferManager* cbManager = nullptr;

	// primary or secondary buffer
	CmdBufferType type;

	// primary command buffer
	vk::CommandBuffer cmdBuffer;
    
    // view port / scissor info
	vk::Viewport viewPort;
	vk::Rect2D scissor;

	// if were using, then store all secondary command buffers for dispatching on each thread
	std::vector<CmdBuffer> secondary;
    
};

}    // namespace VulkanAPI
