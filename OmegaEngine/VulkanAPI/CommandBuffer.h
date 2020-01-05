#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Pipeline.h"

#include <cstdint>
#include <vector>

namespace VulkanAPI
{

// forward decleartions
class PipelineLayout;
class DescriptorSet;
class VkContext;
class CmdBufferManager;
class ShaderProgram;
class RenderPass;
class CmdPool;

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

	CmdBuffer(VkContext& context, const Type type, CmdPool& cmdPool, CmdBufferManager* cbManager = nullptr);
	~CmdBuffer();

	void prepare();
    
    /**
     * @brief This begins the renderpass with the paramters stipulated by the begin info. Also states whether this pass will use secodnary buffers
     */
    void beginPass(const vk::RenderPassBeginInfo& beginInfo, const vk::SubpassContents contents);
    void endPass();
    
	// viewport, scissors, etc.
	void setViewport(const vk::Viewport& viewPort);
	void setScissor(const vk::Rect2D& scissor);

	// primary binding functions
	void bindPipeline(RenderPass* renderpass, ShaderProgram* program);
	void bindDescriptors(ShaderProgram* prog, const Pipeline::Type type);
    void bindDynamicDescriptors(ShaderProgram* prog, std::vector<uint32_t>& offsets, const Pipeline::Type type);
    void bindDynamicDescriptors(ShaderProgram* prog, const uint32_t offset, const Pipeline::Type type);
	void bindPushBlock(ShaderProgram* prog, vk::ShaderStageFlags stage, uint32_t size, void* data);
	void bindVertexBuffer(vk::Buffer buffer, vk::DeviceSize offset);
	void bindIndexBuffer(vk::Buffer buffer, uint32_t offset);

	// dynamic bindings
	void setDepthBias(float biasConstant, float biasClamp, float biasSlope);

	/**
	* @brief Executes all secondary command buffers associated with the primary one
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
    void drawIndexed(size_t indexCount, size_t offset);
	void drawQuad();

	// helper funcs
	vk::CommandBuffer& get()
	{
		return cmdBuffer;
	}

private:
	
	// local vulkan context 
    VkContext& context;
    CmdPool& cmdPool;
	CmdBufferManager* cbManager = nullptr;
    
    // current bindings - variants are used for ease
    Pipeline* boundPipeline = nullptr;
    DescriptorSet* boundDescrSet = nullptr;
    
	// primary or secondary buffer
    Type type;

	vk::CommandBuffer cmdBuffer;
    
    // view port / scissor info
	vk::Viewport viewPort;
	vk::Rect2D scissor;
    
};

}    // namespace VulkanAPI
