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

#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkContext.h"
#include "utility/CString.h"
#include "utility/MurmurHash.h"

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
    Buffer* addUbo(const Util::String& id, const size_t size, VkBufferUsageFlags usage);

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

    // =============== delete buffer =======================================

    void deleteVertexBuffer(VertexBuffer* buffer);

    void deleteIndexBuffer(IndexBuffer* buffer);

    // ======== begin/end frame functions =================================

    bool beginFrame(Swapchain& swapchain);

    void endFrame(Swapchain& swapchain);

    void beginRenderpass(
        CmdBuffer* cmdBuffer,
        RenderPass& rpass,
        FrameBuffer& fbo,
        bool usingSecondaryCommands = false);

    void endRenderpass(CmdBuffer* cmdBuffer);

    // ====== manager helper functions ===================================

    CBufferManager& getCbManager();

    ProgramManager& getProgManager();

    VkContext& getContext();

    VmaAllocator& getVma();

    uint32_t getCurrentImageIndex() const;

    StagingPool& getStagingPool();

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
        // at the moment, usng the same clear flags across all the attachments to keep the key size
        // down
        LoadClearFlags loadOp;
        StoreClearFlags storeOp;
        LoadClearFlags stencilLoadOp;
        StoreClearFlags stencilStoreOp;
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

    static_assert(
        std::is_pod<RPassKey>::value, "RPassKey must be a POD for the hashing to work correctly");
    static_assert(
        std::is_pod<FboKey>::value, "FboKey must be a POD for the hashing to work correctly");

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
    std::unordered_map<RPassKey, std::unique_ptr<RenderPass>, RPassHasher, RPassEqualTo>
        renderPasses;
    std::unordered_map<FboKey, std::unique_ptr<FrameBuffer>, FboHasher, FboEqualTo> frameBuffers;

public:
    RPassKey prepareRPassKey();
    FboKey prepareFboKey();
    RenderPass* findOrCreateRenderPass(
        const RPassKey& key, VulkanAPI::RenderPass::Flags flags = VulkanAPI::RenderPass::Flags::None);
    FrameBuffer* findOrCreateFrameBuffer(const FboKey& key);

public:
    // ================ Texture / Buffer maps ======================

#pragma pack(push, 1)

    // the resource buffers are key'ed by just their id (hashed) as the descriptor update only has
    // access to the id and no other information.
    struct OE_PACKED TextureKey
    {
        uint32_t nameHash;
    };

    struct OE_PACKED BufferKey
    {
        uint32_t nameHash;
    };

#pragma pack(pop)

    static_assert(
        std::is_pod<TextureKey>::value,
        "TextureKey must be a POD for the hashing to work correctly");
    static_assert(
        std::is_pod<BufferKey>::value, "BufferKey must be a POD for the hashing to work correctly");

    using ResourcePtrKey = void*;

    using textureHasher = Util::Murmur3Hasher<TextureKey>;
    using bufferHasher = Util::Murmur3Hasher<BufferKey>;
    using resourcePtrHasher = Util::Murmur3Hasher<ResourcePtrKey>;

    struct TextureEqualTo
    {
        bool operator()(const TextureKey& lhs, const TextureKey& rhs) const
        {
            return lhs.nameHash == rhs.nameHash;
        }
    };

    struct BufferEqualTo
    {
        bool operator()(const BufferKey& lhs, const BufferKey& rhs) const
        {
            return lhs.nameHash == rhs.nameHash;
        }
    };

    struct ResourcePtrEqualTo
    {
        bool operator()(const ResourcePtrKey& lhs, const ResourcePtrKey& rhs) const
        {
            return lhs == rhs;
        }
    };

    // ============= retrieve and delete  resources ============================
    Texture* getTexture2D(const Util::String& key);
    Buffer* getBuffer(const Util::String& key);

    void deleteUbo(const BufferKey& id);
    void deleteTexture(const TextureKey& id);

private:
    // texture/buffer resources
    std::unordered_map<TextureKey, Texture, textureHasher, TextureEqualTo> textures;
    std::unordered_map<BufferKey, Buffer, bufferHasher, BufferEqualTo> buffers;

    // vertex/index buffers
    std::unordered_map<ResourcePtrKey, VertexBuffer*, resourcePtrHasher, ResourcePtrEqualTo>
        vertBuffers;
    std::unordered_map<ResourcePtrKey, IndexBuffer*, resourcePtrHasher, ResourcePtrEqualTo>
        indexBuffers;
};

} // namespace VulkanAPI
