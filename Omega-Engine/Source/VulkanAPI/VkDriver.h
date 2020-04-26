#pragma once

#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkContext.h"
#include "utility/CString.h"
#include "utility/MurmurHash.h"
#include "VulkanAPI/RenderPass.h"

#include <unordered_map>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class ProgramManager;
class CBufferManager;
class Buffer;
class Texture;
class CmdBuffer;
class RenderPass;
class FrameBuffer;
class VertexBuffer;
class IndexBuffer;
class Swapchain;
class SemaphoreManager;

using DynBufferHandle = uint64_t;

class VkDriver
{

public:
    VkDriver();
    ~VkDriver();

    bool createInstance(const char** instanceExt, uint32_t count);

    /// initialises the vulkan driver - includes creating the abstract device, physical device,
    /// queues, etc.
    bool init(const vk::SurfaceKHR surface);

    /// Make sure you call this before closing down the engine!
    void shutdown();

    //  ======= Functions for creating buffers and adding resource data to the backend ======

    /**
     * @brief This is for adding persistant uniform buffers to the backend. These will remain in the
     * backend until the user calls the appropiate destroy function. This function also creates and
     * updates a descriptor set which will be associated with this buffer..
     * @param id This is a string id used to hash and retrieve this buffer
     * @param size The size of the buffer in bytes
     * @param usage Vulkan usage flags depict what this buffer will be used for
     */
    void addUbo(
        const Util::String& id,
        const size_t size,
        VkBufferUsageFlags usage);

    /**
     * @brief Adds a vertex buffer to the vulkan back end. This function also generates the vertex
     * attribute bindings in preperation to using with the relevant pipeline
     */
    VertexBuffer* addVertexBuffer(const size_t size, void* data);

    /**
     * @brief Similiar to the **addVertexBuffer** function, adds a index buffer to the vulkan
     * backend. Note: it is presumed to be of the type uint32_t.
     */
    IndexBuffer* addIndexBuffer(const size_t size, uint32_t* data);
    
    Texture* findOrCreateTexture2d(
    const Util::String& id,
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    const uint8_t faceCount,
    const uint8_t arrayCount,
    vk::ImageUsageFlags usageFlags);
    
    Texture* findOrCreateTexture2d(
        const Util::String& id,
        vk::Format format,
        const uint32_t width,
        const uint32_t height,
        const uint8_t mipLevels,
        vk::ImageUsageFlags usageFlags);

    // ============== buffer update functions ===============================

    void update2DTexture(const Util::String& id, void* data);

    void updateUbo(const Util::String& id, const size_t size, void* data);

    // ============= retrieve resources ====================================

    Texture* getTexture2D(const Util::String& name);

    Buffer* getBuffer(const Util::String& name);

    // =============== delete buffer =======================================

    void deleteUbo(const Util::String& id);

    void deleteVertexBuffer(VertexBuffer* buffer);

    void deleteIndexBuffer(IndexBuffer* buffer);

    // ======== begin/end frame functions =================================

    void beginFrame(Swapchain& swapchain);

    void endFrame(Swapchain& swapchain);

    void beginRenderpass(CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbo, bool usingSecondaryCommands = false);

    void endRenderpass(CmdBuffer* cmdBuffer);

    // ====== manager helper functions ===================================

    CBufferManager& getCbManager();

    ProgramManager& getProgManager();

    VkContext& getContext();

    VmaAllocator& getVma();

    uint32_t getCurrentImageIndex() const;
    
private:
    // managers
    std::unique_ptr<ProgramManager> progManager;
    std::unique_ptr<CBufferManager> cbManager;

    // the current device context
    VkContext context;

    // external mem allocator
    VmaAllocator vmaAlloc;

    // staging pool used for managing CPU stages
    std::unique_ptr<StagingPool> stagingPool;

    // The current present KHR frame image index
    uint32_t imageIndex = 0;

    // used for ensuring that the image has completed
    vk::Semaphore imageReadySemaphore;
 
public:
    
    // ================ Frame buffer cache =========================
    #pragma pack(push, 1)

    struct OE_PACKED RPassKey
    {
        vk::ImageLayout finalLayout[6];
        vk::Format colourFormats[6];
        // at the moment, usng the same clear flags acroos all the attachments to keep the key size down
        RenderPass::LoadClearFlags loadOp;
        RenderPass::StoreClearFlags storeOp;
        RenderPass::LoadClearFlags stencilLoadOp;
        RenderPass::StoreClearFlags stencilStoreOp;
        vk::Format depth;
    };
    
    struct OE_PACKED FboKey
    {
        VkRenderPass renderpass;
        VkImageView views[6];
        uint32_t width;
        uint32_t height;
    };
    #pragma pack(pop)
    
    static_assert(std::is_pod<RPassKey>::value, "RPassKey must be a POD for the hashing to work correctly");
    static_assert(std::is_pod<FboKey>::value, "FboKey must be a POD for the hashing to work correctly");
    
    using RPassHasher = Util::Murmur3Hasher<RPassKey>;
    using FboHasher = Util::Murmur3Hasher<FboKey>;

    struct RPassEqualTo
    {
        bool operator()(const RPassKey& lhs, const RPassKey& rhs) const;
    };
    
    struct FboEqualTo
    {
        bool operator()(const FboKey& lhs, const FboKey& rhs) const;
    };
    
private:
    
    std::unordered_map<RPassKey, std::unique_ptr<RenderPass>, RPassHasher, RPassEqualTo> renderPasses;
    std::unordered_map<FboKey, std::unique_ptr<FrameBuffer>, FboHasher, FboEqualTo> frameBuffers;
    
public:
    
    RPassKey prepareRPassKey();
    FboKey prepareFboKey();
    RenderPass* findOrCreateRenderPass(const RPassKey& key);
    FrameBuffer* findOrCreateFrameBuffer(const FboKey& key);
    
private:
    
    // ================ Texture / Buffer maps ======================

    using ResourceIdKey = Util::String;
    using ResourcePtrKey = void*;

    using resourceIdHasher = Util::Murmur3Hasher<ResourceIdKey>;
    using resourcePtrHasher = Util::Murmur3Hasher<ResourcePtrKey>;
    
    struct ResourceIdEqualTo
    {
        bool operator()(const ResourceIdKey& lhs, const ResourceIdKey& rhs) const
        {
            return lhs == rhs;
        }
    };

    struct ResourcePtrEqualTo
    {
        bool operator()(const ResourcePtrKey& lhs, const ResourcePtrKey& rhs) const
        {
            return lhs == rhs;
        }
    };
    
    // texture/buffer resources
    std::unordered_map<ResourceIdKey, Texture, resourceIdHasher, ResourceIdEqualTo> textures;
    std::unordered_map<ResourceIdKey, Buffer, resourceIdHasher, ResourceIdEqualTo> buffers;
    
    // vertex/index buffers
    std::unordered_map<ResourcePtrKey, VertexBuffer*, resourcePtrHasher, ResourcePtrEqualTo> vertBuffers;
    std::unordered_map<ResourcePtrKey, IndexBuffer*, resourcePtrHasher, ResourcePtrEqualTo> indexBuffers;
};

} // namespace VulkanAPI
