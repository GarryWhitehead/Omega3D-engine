/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
    void beginSecondary(RenderPass& renderpass, FrameBuffer& fbo);
    void end();

    // viewport, scissors, etc.
    void setViewport(const vk::Viewport& viewPort);
    void setScissor(const vk::Rect2D& scissor);

    // primary binding functions
    void bindPipeline(
        CBufferManager& cbManager,
        RenderPass* renderpass,
        FrameBuffer* fbo,
        ShaderProgram* program,
        Pipeline::Type type);

    void bindPipeline(Pipeline& pipeline);
    
    void bindDescriptors(
        CBufferManager& cbManager, ShaderProgram* prog, const Pipeline::Type pipelineType);
    
    void bindDescriptors(const vk::PipelineLayout layout, std::vector<vk::DescriptorSet>& sets, const Pipeline::Type pipelineType);
    
    void bindDescriptors(
        CBufferManager& cbManager,
        ShaderProgram* prog,
        std::vector<uint32_t>& offsets,
        const Pipeline::Type type);
    
    void bindDescriptors(const vk::PipelineLayout layout, std::vector<vk::DescriptorSet>& sets, std::vector<uint32_t>& offsets, const Pipeline::Type pipelineType);
    
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

    void resetPool();
    void resetCmdBuffer();

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
    friend class VkDriver;

private:
    
    // these are private as the user should use the begin and end renderpass functions found in the driver
    void beginPass(const vk::RenderPassBeginInfo& beginInfo, const vk::SubpassContents contents);
    void endPass();
    
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
    bool workSubmitted = false;

    // view port / scissor info
    vk::Viewport viewPort;
    vk::Rect2D scissor;
};

} // namespace VulkanAPI
