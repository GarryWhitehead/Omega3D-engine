#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Pipeline.h"
#include "utility/Logger.h"

#include <cstdint>
#include <vector>

namespace VulkanAPI
{

// forward decleartions
class PipelineLayout;
struct VkContext;
class CBufferManager;
class ShaderProgram;
class RenderPass;
class FrameBuffer;
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

    CmdBuffer() = delete;
    CmdBuffer(VkContext& context, vk::CommandPool& cmdPool, const Type type);
    ~CmdBuffer();

    void init();
    void begin();
    void beginSecondary(RenderPass& renderpass, FrameBuffer& fbuffer);
    void end();

    /**
     * @brief This begins the renderpass with the paramters stipulated by the begin info. Also
     * states whether this pass will use secodnary buffers
     */
    void beginPass(const vk::RenderPassBeginInfo& beginInfo, const vk::SubpassContents contents);
    void endPass();
    
    // viewport, scissors, etc.
    void setViewport(const vk::Viewport& viewPort);
    void setScissor(const vk::Rect2D& scissor);

    // primary binding functions
    void bindPipeline(CBufferManager& cbManager, RenderPass* renderpass, ShaderProgram* program);
    void bindDescriptors(CBufferManager& cbManager, ShaderProgram* prog, const Pipeline::Type pipelineType);
    void bindDynamicDescriptors(CBufferManager& cbManager,
        ShaderProgram* prog, std::vector<uint32_t>& offsets, const Pipeline::Type type);
    void
    bindDynamicDescriptors(CBufferManager& cbManager, ShaderProgram* prog, const uint32_t offset, const Pipeline::Type type);
    void bindPushBlock(ShaderProgram* prog, vk::ShaderStageFlags stage, uint32_t size, void* data);
    void bindVertexBuffer(vk::Buffer buffer, vk::DeviceSize offset);
    void bindIndexBuffer(vk::Buffer buffer, uint32_t offset);

    // dynamic bindings
    void setDepthBias(float biasConstant, float biasClamp, float biasSlope);

    /**
     * @brief Flushes the queue that this cmd buffer is associated with.
     */
    void flush();

    /**
     * @brief Submits this cmd buffer to the specified queue
     */
    void submit(vk::Semaphore& waitSemaphore, vk::Semaphore& signalSemaphore, vk::Fence& fence);

    void reset();

    // drawing functions
    void drawIndexed(uint32_t indexCount);
    void drawIndexed(uint32_t indexCount, int32_t offset);
    void drawQuad();

    // helper funcs
    vk::CommandBuffer& get()
    {
        return cmdBuffer;
    }

    friend class CBufferManager;

private:
    // local vulkan context
    VkContext& context;

    vk::CommandPool& cmdPool;

    // current bindings - variants are used for ease
    Pipeline* boundPipeline = nullptr;

    /// the queue to use for this pool
    uint32_t queueIndex;

    // primary or secondary buffer
    Type type;

    vk::CommandBuffer cmdBuffer;

    // a command buffer has its own internal fence for syncing between frames
    vk::Fence cmdFence;

    // view port / scissor info
    vk::Viewport viewPort;
    vk::Rect2D scissor;
};

} // namespace VulkanAPI
