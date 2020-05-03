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

#include "VkDriver.h"

#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/Logger.h"
#include "utility/MurmurHash.h"

#include <cassert>

namespace VulkanAPI
{

// ========== Framebuffer / RenderPass cache ===============

bool VkDriver::RPassEqualTo::operator()(const RPassKey& lhs, const RPassKey& rhs) const
{
    int matchFormat = memcmp(lhs.colourFormats, rhs.colourFormats, sizeof(vk::Format) * 6);
    int matchLayout = memcmp(lhs.finalLayout, rhs.finalLayout, sizeof(vk::ImageLayout) * 6);
    bool isEqual = matchFormat == 0 && matchLayout == 0;

    return isEqual && lhs.depth == rhs.depth && lhs.storeOp == rhs.storeOp &&
        lhs.loadOp == rhs.loadOp && lhs.stencilStoreOp == rhs.stencilStoreOp &&
        lhs.stencilLoadOp == rhs.stencilLoadOp;
}

bool VkDriver::FboEqualTo::operator()(const FboKey& lhs, const FboKey& rhs) const
{
    int match = memcmp(lhs.views, rhs.views, sizeof(VkImageView) * 6);
    bool isEqual = match == 0;

    return lhs.width == rhs.width && lhs.height == rhs.height && isEqual &&
        lhs.renderpass == rhs.renderpass;
}

RenderPass* VkDriver::findOrCreateRenderPass(const RPassKey& key)
{
    auto iter = renderPasses.find(key);
    if (iter == renderPasses.end())
    {
        // create a new renderpass
        auto rpass = std::make_unique<RenderPass>(context);

        // add the colour attachments
        uint32_t idx = 0;
        while (key.colourFormats[idx] != vk::Format(0))
        {
            rpass->addAttachment(
                key.colourFormats[idx],
                1,
                key.finalLayout[idx],
                key.loadOp,
                key.storeOp,
                key.stencilLoadOp,
                key.stencilStoreOp);
            ++idx;
        }
        if (key.depth != vk::Format(0))
        {
            // TODO: the clear flags should be made user definable
            rpass->addAttachment(
                key.depth,
                1,
                vk::ImageLayout::eDepthStencilReadOnlyOptimal,
                LoadClearFlags::Clear,
                StoreClearFlags::DontCare,
                LoadClearFlags::DontCare,
                StoreClearFlags::DontCare);
        }
        rpass->prepare();

        renderPasses.emplace(key, std::move(rpass));
        return renderPasses[key].get();
    }

    return iter->second.get();
}

FrameBuffer* VkDriver::findOrCreateFrameBuffer(const FboKey& key)
{
    auto iter = frameBuffers.find(key);
    if (iter == frameBuffers.end())
    {
        // create a new framebuffer
        auto fbo = std::make_unique<FrameBuffer>(context);

        std::vector<vk::ImageView> imageViews;
        imageViews.reserve(6);

        for (uint32_t idx = 0; idx < 6; ++idx)
        {
            if (!key.views[idx])
            {
                break;
            }
            imageViews.emplace_back(vk::ImageView(key.views[idx]));
        }
        fbo->create(key.renderpass, imageViews, key.width, key.height);

        frameBuffers.emplace(key, std::move(fbo));
        return frameBuffers[key].get();
    }

    return iter->second.get();
}

VkDriver::RPassKey VkDriver::prepareRPassKey()
{
    RPassKey rpassKey;

    memset(rpassKey.colourFormats, 0, sizeof(vk::Format) * 6);
    memset(rpassKey.finalLayout, 0, sizeof(vk::ImageLayout) * 6);
    rpassKey.depth = vk::Format(0);
    rpassKey.loadOp = VulkanAPI::LoadClearFlags::Clear;
    rpassKey.storeOp = VulkanAPI::StoreClearFlags::Store;
    rpassKey.stencilLoadOp = VulkanAPI::LoadClearFlags::DontCare;
    rpassKey.stencilStoreOp = VulkanAPI::StoreClearFlags::DontCare;

    return rpassKey;
}

VkDriver::FboKey VkDriver::prepareFboKey()
{
    FboKey key;
    memset(key.views, 0, sizeof(VkImageView) * 6);
    key.width = 0;
    key.height = 0;
    return key;
}

// =================== driver ==============================

VkDriver::VkDriver() : progManager(std::make_unique<ProgramManager>(*this))
{
}

VkDriver::~VkDriver()
{
    shutdown();
}

bool VkDriver::createInstance(const char** instanceExt, uint32_t count)
{
    // create a new vulkan instance
    if (!context.createInstance(instanceExt, count))
    {
        return false;
    }
    return true;
}

bool VkDriver::init(const vk::SurfaceKHR surface)
{
    // prepare the vulkan backend
    // prepare the physical and abstract device including queues
    if (!context.prepareDevice(surface))
    {
        return false;
    }

    // set up the memory allocator
    VmaAllocatorCreateInfo createInfo = {};
    createInfo.physicalDevice = context.physical;
    createInfo.device = context.device;
    vmaCreateAllocator(&createInfo, &vmaAlloc);

    // we can now create the staging pool now we have the VMA up and running....
    stagingPool = std::make_unique<StagingPool>(vmaAlloc);

    // and the command buffer manager - note: the pool is init on construction so must be done after
    // driver init
    cbManager = std::make_unique<CBufferManager>(*this);

    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_CHECK_RESULT(
        context.device.createSemaphore(&semaphoreCreateInfo, nullptr, &imageReadySemaphore));

    return true;
}

void VkDriver::shutdown()
{
    context.device.destroy(imageReadySemaphore, nullptr);
    vmaDestroyAllocator(vmaAlloc);
}

// =========== functions for buffer/texture creation ================

Buffer* VkDriver::addUbo(const Util::String& id, const size_t size, VkBufferUsageFlags usage)
{
    uint32_t idKey = Util::murmurHash3((uint32_t*) id.c_str(), id.size(), 0);
    BufferKey key = {idKey};

    // check if the buffer already exists with the same id. If so, check the size of the current
    // buffer against the size of the requested buffer. If the space is too small, destroy the
    // existing buffer and create a new one.
    auto iter = buffers.find(key);
    if (iter != buffers.end())
    {
        // check the size
        Buffer& buffer = iter->second;
        if (size < buffer.getSize())
        {
            // nothing else to do here as the buffer is of adequate size
            return &iter->second;
        }
        deleteUbo(key);
    }

    Buffer buffer;
    buffer.prepare(vmaAlloc, static_cast<VkDeviceSize>(size), usage);
    buffers.emplace(key, buffer);
    return &buffers[key];
}

Texture* VkDriver::findOrCreateTexture2d(
    const Util::String& id,
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    const uint8_t faceCount,
    const uint8_t arrayCount,
    vk::ImageUsageFlags usageFlags)
{
    uint32_t idKey = Util::murmurHash3((uint32_t*) id.c_str(), id.size(), 0);
    TextureKey key {idKey};

    auto iter = textures.find(key);
    if (iter != textures.end())
    {
        return &iter->second;
    }

    Texture tex;
    tex.create2dTex(*this, format, width, height, mipLevels, faceCount, arrayCount, usageFlags);
    textures.emplace(key, std::move(tex));
    LOGGER_INFO("Adding 2D texture with id: %s\n", id.c_str());
    return &textures[key];
}

Texture* VkDriver::findOrCreateTexture2d(
    const Util::String& id,
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    vk::ImageUsageFlags usageFlags)
{
    return findOrCreateTexture2d(id, format, width, height, mipLevels, 1, 1, usageFlags);
}

VertexBuffer* VkDriver::addVertexBuffer(const size_t size, void* data)
{
    assert(data);
    VertexBuffer* buffer = new VertexBuffer;
    buffer->create(*this, vmaAlloc, *stagingPool, data, size);
    vertBuffers.emplace(static_cast<void*>(buffer), buffer);
    return buffer;
}

IndexBuffer* VkDriver::addIndexBuffer(const size_t size, uint32_t* data)
{
    assert(data);
    IndexBuffer* buffer = new IndexBuffer;
    buffer->create(*this, vmaAlloc, *stagingPool, data, size);
    indexBuffers.emplace(static_cast<void*>(buffer), buffer);
    return buffer;
}

// ============================ delete resources ==============================================

void VkDriver::deleteUbo(const BufferKey& key)
{
    auto iter = buffers.find(key);
    assert(iter != buffers.end());
    context.device.destroy(iter->second.get(), nullptr);
    buffers.erase(key);
}

void VkDriver::deleteTexture(const TextureKey& key)
{
    auto iter = textures.find(key);
    assert(iter != textures.end());
    textures.erase(key);
}

void VkDriver::deleteVertexBuffer(VertexBuffer* buffer)
{
    auto iter = vertBuffers.find({buffer});
    assert(iter != vertBuffers.end());
    context.device.destroy(iter->second->get(), nullptr);
    vertBuffers.erase({buffer});
    delete buffer;
    buffer = nullptr;
}

void VkDriver::deleteIndexBuffer(IndexBuffer* buffer)
{
    auto iter = indexBuffers.find({buffer});
    assert(iter != indexBuffers.end());
    context.device.destroy(iter->second->get(), nullptr);
    indexBuffers.erase({buffer});
    delete buffer;
    buffer = nullptr;
}

// ======================== resource retrieval ===================================

Texture* VkDriver::getTexture2D(const Util::String& id)
{
    uint32_t key = Util::murmurHash3((uint32_t*) id.c_str(), id.size(), 0);
    auto iter = textures.find({key});
    if (iter == textures.end())
    {
        return nullptr;
    }
    return &iter->second;
}

Buffer* VkDriver::getBuffer(const Util::String& id)
{
    uint32_t key = Util::murmurHash3((uint32_t*) id.c_str(), id.size(), 0);
    auto iter = buffers.find({key});
    if (iter == buffers.end())
    {
        return nullptr;
    }
    return &iter->second;
}

// ============ begin/end frame functions ======================

void VkDriver::beginFrame(Swapchain& swapchain)
{
    cbManager->resetSecondaryCommands();

    // get the next image index which will be the framebuffer we draw too
    context.device.acquireNextImageKHR(
        swapchain.get(),
        std::numeric_limits<uint64_t>::max(),
        imageReadySemaphore,
        {},
        &imageIndex);
}

void VkDriver::endFrame(Swapchain& swapchain)
{
    // submit the swap cmd buffer and present
    cbManager->flushSwapchainCmdBuffer(imageReadySemaphore, swapchain, imageIndex);
}

void VkDriver::beginRenderpass(
    CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbo, bool clearAttachments, bool usingSecondaryCommands)
{
    std::vector<vk::ClearValue> clearValues;
    
    if (clearAttachments)
    {
        // setup the clear values for this pass - need one for each attachment
        auto& attachments = rpass.getAttachments();
        clearValues.resize(attachments.size());

        for (size_t i = 0; i < attachments.size(); ++i)
        {
            if (VkUtil::isDepth(attachments[i].format) || VkUtil::isStencil(attachments[i].format))
            {
                clearValues[attachments.size() - 1].depthStencil =
                    vk::ClearDepthStencilValue {1.0f, 0};
            }
            else
            {
                clearValues[i].color.float32[0] = rpass.clearCol.r;
                clearValues[i].color.float32[1] = rpass.clearCol.g;
                clearValues[i].color.float32[2] = rpass.clearCol.b;
                clearValues[i].color.float32[3] = rpass.clearCol.a;
            }
        }
    }

    // extents of the frame buffer
    vk::Rect2D extents {{0, 0}, {fbo.getWidth(), fbo.getHeight()}};

    vk::RenderPassBeginInfo beginInfo {rpass.get(),
                                       fbo.get(),
                                       extents,
                                       clearAttachments ? static_cast<uint32_t>(clearValues.size()) : 0,
                                       clearAttachments ? clearValues.data() : nullptr};

    vk::SubpassContents contents = usingSecondaryCommands
        ? vk::SubpassContents::eSecondaryCommandBuffers
        : vk::SubpassContents::eInline;
    cmdBuffer->beginPass(beginInfo, contents);

    // the viewport and scissor aren't set in the primary command buffer if using secondarys
    if (!usingSecondaryCommands)
    {
        // use custom defined viewing area - at the moment set to the framebuffer size
        vk::Viewport viewport {0.0f,
                               0.0f,
                               static_cast<float>(fbo.getWidth()),
                               static_cast<float>(fbo.getHeight()),
                               0.0f,
                               1.0f};

        cmdBuffer->setViewport(viewport);

        vk::Rect2D scissor {
            {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
            {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}};

        cmdBuffer->setScissor(scissor);
    }
}

void VkDriver::endRenderpass(CmdBuffer* cmdBuffer)
{
    cmdBuffer->endPass();
}

CBufferManager& VkDriver::getCbManager()
{
    return *cbManager;
}

ProgramManager& VkDriver::getProgManager()
{
    return *progManager;
}

VkContext& VkDriver::getContext()
{
    return context;
}

VmaAllocator& VkDriver::getVma()
{
    return vmaAlloc;
}

uint32_t VkDriver::getCurrentImageIndex() const
{
    return imageIndex;
}

StagingPool& VkDriver::getStagingPool()
{
    return *stagingPool.get();
}

} // namespace VulkanAPI
