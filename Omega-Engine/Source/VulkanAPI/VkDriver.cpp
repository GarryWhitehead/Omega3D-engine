#include "VkDriver.h"

#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/Logger.h"

#include <cassert>

namespace VulkanAPI
{

VkDriver::VkDriver() : progManager(std::make_unique<ProgramManager>(context))
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
    cbManager = std::make_unique<CBufferManager>(context);

    // create the semaphore for signalling a new frame is ready now
    beginSemaphore = spManager->getSemaphore();

    return true;
}

void VkDriver::shutdown()
{
    vmaDestroyAllocator(vmaAlloc);
}

// =========== functions for buffer/texture creation ================

void VkDriver::addUbo(
    const Util::String& id, const size_t size, VkBufferUsageFlags usage, bool updateDescr)
{
    // check if the buffer already exists with the same id. If so, check the size of the current
    // buffer against the size of the requested buffer. If the space is too small, destroy the
    // existing buffer and create a new one.
    auto iter = buffers.find({id.c_str()});
    if (iter != buffers.end())
    {
        // check the size
        Buffer& buffer = iter->second;
        if (size < buffer.getSize())
        {
            // nothing else to do here as the buffer is of adequate size
            return;
        }
        deleteUbo(id);
    }

    Buffer buffer;
    buffer.prepare(vmaAlloc, static_cast<VkDeviceSize>(size), usage);
    VkHash::ResourceIdKey key {id.c_str()};
    buffers.emplace(key, buffer);

    if (updateDescr)
    {
        cbManager->updateDescriptors(id, buffers[{id.c_str()}]);
    }
}

void VkDriver::add2DTexture(
    const Util::String& id,
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    vk::ImageUsageFlags usageFlags,
    bool updateDescr)
{
    // for textures, we expect the ids to be unique.
    auto iter = textures.find({id.c_str()});
    assert(iter == textures.end());

    Texture tex;
    tex.create2dTex(*this, format, width, height, mipLevels, usageFlags);
    VkHash::ResourceIdKey key {id.c_str()};
    textures.emplace(key, std::move(tex));

    if (updateDescr)
    {
        cbManager->updateDescriptors(id, tex);
    }
}

VertexBuffer* VkDriver::addVertexBuffer(const size_t size, void* data)
{
    assert(data);
    VertexBuffer* buffer = new VertexBuffer;
    buffer->create(*this, vmaAlloc, *stagingPool, data, size);
    VkHash::ResourcePtrKey key = {static_cast<void*>(buffer)};
    vertBuffers.emplace(key, buffer);
    return buffer;
}

IndexBuffer* VkDriver::addIndexBuffer(const size_t size, uint32_t* data)
{
    assert(data);
    IndexBuffer* buffer = new IndexBuffer;
    buffer->create(*this, vmaAlloc, *stagingPool, data, size);
    VkHash::ResourcePtrKey key = {static_cast<void*>(buffer)};
    indexBuffers.emplace(key, buffer);
    return buffer;
}

// ========================= resource updates ===================================================

void VkDriver::update2DTexture(const Util::String& id, void* data)
{
    auto iter = textures.find({id.c_str()});
    assert(iter != textures.end());
    assert(data);

    iter->second.map(*this, *stagingPool, data);
}

void VkDriver::updateUbo(const Util::String& id, const size_t size, void* data)
{
    auto iter = buffers.find({id.c_str()});
    assert(iter != buffers.end());
    assert(data);

    iter->second.map(data, size);
}

// ============================ delete resources ==============================================

void VkDriver::deleteUbo(const Util::String& id)
{
    auto iter = buffers.find({id.c_str()});
    assert(iter != buffers.end());
    context.device.destroy(iter->second.get(), nullptr);
    buffers.erase({id.c_str()});
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

Texture* VkDriver::getTexture2D(const Util::String& name)
{
    auto iter = textures.find({name.c_str()});
    if (iter == textures.end())
    {
        return nullptr;
    }
    return &iter->second;
}

Buffer* VkDriver::getBuffer(const Util::String& name)
{
    auto iter = buffers.find({name.c_str()});
    if (iter == buffers.end())
    {
        return nullptr;
    }
    return &iter->second;
}

// ============ begin/end frame functions ======================

void VkDriver::beginFrame(Swapchain& swapchain)
{
    // TODO: need to reset some stuff here!

    // get the next image index which will be the framebuffer we draw too
    assert(beginSemaphore);
    context.device.acquireNextImageKHR(
        swapchain.get(), std::numeric_limits<uint64_t>::max(), beginSemaphore, {}, &imageIndex);
}

void VkDriver::endFrame(Swapchain& swapchain)
{
    // submit all of the cmd buffers - it's supposedly a good idea to keep the number of cmd buffers
    // to a min, as this can impact on performance(?)
    cbManager->submitFrame(swapchain, imageIndex, beginSemaphore);
}

void CBufferManager::beginRenderpass(CmdBuffer* cmdBuffer, RenderPass& rpass, FrameBuffer& fbuffer)
{
    // setup the clear values for this pass - need one for each attachment
    vk::ClearValue clearValue[2];

    if (rpass.hasColourAttach())
    {
        clearValue[0].color.float32[0] = rpass.clearCol.r;
        clearValue[0].color.float32[1] = rpass.clearCol.g;
        clearValue[0].color.float32[2] = rpass.clearCol.b;
        clearValue[0].color.float32[3] = rpass.clearCol.a;
    }
    if (rpass.hasDepthAttach())
    {
        clearValue[1].depthStencil = vk::ClearDepthStencilValue {rpass.depthClear, 0};
    }

    // extents of the frame buffer
    vk::Rect2D extents {{0, 0}, {fbuffer.getWidth(), fbuffer.getHeight()}};

    vk::RenderPassBeginInfo beginInfo {rpass.get(), fbuffer.get(), extents, 1, clearValue};
    cmdBuffer->beginPass(beginInfo, vk::SubpassContents::eInline);

    // use custom defined viewing area - at the moment set to the framebuffer size
    vk::Viewport viewport {0.0f,
                           0.0f,
                           static_cast<float>(fbuffer.getWidth()),
                           static_cast<float>(fbuffer.getHeight()),
                           0.0f,
                           1.0f};

    cmdBuffer->setViewport(viewport);

    vk::Rect2D scissor {
        {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
        {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}};

    cmdBuffer->setScissor(scissor);
}

void CBufferManager::endRenderpass(CmdBuffer* cmdBuffer)
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
    assert(imageIndex != UINT32_MAX);
    return imageIndex;
}

} // namespace VulkanAPI
