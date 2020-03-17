#pragma once

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/SemaphoreManager.h"
#include "VulkanAPI/Shader.h"
#include "utility/MurmurHash.h"

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{
// forward declerations
class RenderPass;
struct VkContext;
class Pipeline;
class CmdBuffer;
class ShaderProgram;
class FrameBuffer;
class Swapchain;

using CmdBufferHandle = uint64_t;

// A blueprint of all descriptors bound to each shader. When creating a set or updating info we can
// use this information for the update.
struct DescriptorBinding
{
    Util::String name;
    uint32_t set;
    vk::DescriptorSetLayoutBinding binding;
};

struct DescriptorSetInfo
{
    vk::DescriptorSetLayout layout;
    vk::DescriptorSet descrSet;
    uint16_t setValue;
};

class CBufferManager
{
public:
    // some erbitarty numbers which need monitoring for possible issues due to overflow
    constexpr static const uint32_t MaxDescriptorPoolSize = 50;
    constexpr static const uint32_t MaxDescriptorPoolSets = 10;

    struct ThreadedCmdBuffer
    {
        std::unique_ptr<CmdBuffer> secondary;
        vk::CommandPool cmdPool;
    };

    CBufferManager(VkContext& context);
    ~CBufferManager();

    // not copyable
    CBufferManager(const CBufferManager&) = delete;
    CBufferManager& operator=(const CBufferManager) = delete;

    /**
     * @brief Checks whether a piepline exsists baseed on the specified hash. Returns a pointer to
     * the pipeline if it does, otherwise nullptr
     */
    Pipeline* findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass);

    void addDescriptorLayout(
        Util::String shaderId,
        Util::String layoutId,
        uint32_t set,
        uint32_t binding,
        vk::DescriptorType bindType,
        vk::ShaderStageFlags flags);

    DescriptorSetInfo* findDescriptorSet(const Util::String& id, const uint8_t setValue);
    std::vector<DescriptorSetInfo> findDescriptorSets(const Util::String& id);

    void buildDescriptorSets();

    void createMainDescriptorPool();

    bool updateDescriptors(const Util::String& id, Buffer& buffer);
    bool updateDescriptors(const Util::String& id, Texture& tex);
    void updateDescriptors(
        const uint32_t bindingValue,
        const vk::DescriptorType& type,
        const vk::DescriptorSet& set,
        Texture& tex);

    // returns the work commands buffer used for transient work such as buffer copying, etc.
    CmdBuffer* getWorkCmdBuffer();

    // this is used by the main thread (mainly by the rendergraph). The user must take control of
    // all cmd buffer actions including the beginning and ending of the buffer.
    CmdBuffer* getCmdBuffer();

    // flush the main primary commands to the device queue
    void flushCmdBuffer();

    void flushSwapchainCmdBuffer(
        vk::Semaphore& imageReadySemaphore, Swapchain& swapchain, const uint32_t imageIndex);

    vk::DescriptorPool& getDescriptorPool();

    // =============== renderpass functions ================================

    void beginRenderpass(CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbuffer);

    void endRenderpass(CmdBuffer* cmdBuffer);

    CmdBuffer* createSecondaryCmdBuffer();
    void executeSecondaryCommands();

    friend class CmdBuffer;

private:
    // The current vulkan context
    VkContext& context;

    // the main command pool - only to be used on the main thread
    vk::CommandPool cmdPool;

    // to stop the over creation of cmd buffers whcih can hinder performace (maybe?) only three cmd
    // buffer are allowed: main thread commands, swapchain and a worker cmd buffer for tasks such as
    // buffer copying, etc.
    std::unique_ptr<CmdBuffer> cmdBuffer;
    std::unique_ptr<CmdBuffer> scCmdBuffer;
    std::unique_ptr<CmdBuffer> workCmdBuffer;

    // one threaded cmd buffer per thread - the inherited buffer will always be the main cmd buffer
    std::vector<ThreadedCmdBuffer> threadedBuffers;

    vk::DescriptorPool descriptorPool;

    // semaphores for the final presentation
    vk::Semaphore renderingCompleteSemaphore;

private:
    // =============== pipeline hasher ======================
    struct PLineKey
    {
        // first three comprise the shader hash
        ShaderProgram* prog;
        RenderPass* pass;
    };

    using PLineHasher = Util::Murmur3Hasher<PLineKey>;

    struct PLineEqual
    {
        bool operator()(const PLineKey& lhs, const PLineKey& rhs) const
        {
            return lhs.prog == rhs.prog && lhs.pass == rhs.pass;
        }
    };

    // graphic pipelines are stored in the cmd buffer for the reason that they are inextricably
    // linked to the cmd buffer during draw operations. The majority of the required data comes from
    // the shader, but due to each pipeline being exclusively tied to a renderpass, we can only
    // create the pipeline once these have been created.
    std::unordered_map<PLineKey, Pipeline*, PLineHasher, PLineEqual> pipelines;

private:
    // descriptor layouts are ordered by the shader id name (usually the filename)
    std::unordered_map<Util::String, std::vector<DescriptorBinding>> descriptorBindings;

    struct DescriptorKey
    {
        // the id will be typically the shader filename
        Util::String id;
    };

    using DescriptorHasher = Util::Murmur3Hasher<DescriptorKey>;

    struct DescriptorEqual
    {
        bool operator()(const DescriptorKey& lhs, const DescriptorKey& rhs) const
        {
            return lhs.id.compare(rhs.id);
        }
    };

    std::unordered_map<
        DescriptorKey,
        std::vector<DescriptorSetInfo>,
        DescriptorHasher,
        DescriptorEqual>
        descriptorSets;
};


} // namespace VulkanAPI
