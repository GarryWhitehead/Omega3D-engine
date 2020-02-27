#pragma once

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/SemaphoreManager.h"
#include "VulkanAPI/Shader.h"

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
// forward declerations
class RenderPass;
class VkContext;
class Pipeline;
class CmdBuffer;
class ShaderProgram;
class FrameBuffer;
class Swapchain;

using CmdBufferHandle = uint64_t;

class CBufferManager
{
public:

    struct ThreadedCmdBuffer
    {
        CmdBuffer secondary;
        vk::CommandPool cmdPool;
    };

    CBufferManager(VkContext& context, SemaphoreManager& spManager);
    ~CBufferManager();

    // not copyable
    CBufferManager(const CBufferManager&) = delete;
    CBufferManager& operator=(const CBufferManager) = delete;

    /**
     * @brief Checks whether a piepline exsists baseed on the specified hash. Returns a pointer to
     * the pipeline if it does, otherwise nullptr
     */
    Pipeline* findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass);

    void beginNewFame();

    void submitFrame(
        Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore);

    // =============== renderpass functions ================================

    void beginRenderpass(CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbuffer);

    void endRenderpass(CmdBuffer* cmdBuffer);

    friend class CmdBuffer;

private:
    // The current vulkan context
    VkContext& context;

    // the main command pool - only to be used on the main thread
    vk::CommandPool cmdPool;

    // to stop the over creation of cmd buffers whcih can hinder performace (maybe?) only three cmd buffer are allowed:
    // main thread commands, swapchain and a worker cmd buffer for tasks such as buffer copying, etc.
    CmdBuffer cmdBuffer;
    CmdBuffer scCmdBuffer;
    CmdBuffer workCmdBuffer;

    // one threaded cmd buffer per thread - the inherited buffer will always be the main cmd buffer
    std::vector<ThreadedCmdBuffer> threadedBuffers;

private:
    // =============== pipeline hasher ======================
    struct PLineHash
    {
        // first three comprise the shader hash
        ShaderProgram* prog;
        RenderPass* pass;
    };

    struct PLineHasher
    {
        size_t operator()(PLineHash const& id) const noexcept
        {
            size_t h1 = std::hash<ShaderProgram*> {}(id.prog);
            size_t h2 = std::hash<RenderPass*> {}(id.pass);
            return h1 ^ (h2 << 1);
        }
    };

    struct PLineEqual
    {
        bool operator()(const PLineHash& lhs, const PLineHash& rhs) const
        {
            return lhs.prog == rhs.prog && lhs.pass == rhs.pass;
        }
    };

    // graphic pipelines are stored in the cmd buffer for the reason that they are inextricably
    // linked to the cmd buffer during draw operations. The majority of the required data comes from
    // the shader, but due to each pipeline being exclusively tied to a renderpass, we can only
    // create the pipeline once these have been created.
    std::unordered_map<PLineHash, Pipeline*, PLineHasher, PLineEqual> pipelines;
};

} // namespace VulkanAPI
