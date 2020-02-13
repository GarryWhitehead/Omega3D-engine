#include "VkDriver.h"

#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/Logger.h"

#include <cassert>

namespace VulkanAPI
{

VkDriver::VkDriver()
    : progManager(std::make_unique<ProgramManager>(context))
    , spManager(std::make_unique<SemaphoreManager>(context))
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
    cbManager = std::make_unique<CmdBufferManager>(context, *spManager);

    // create the semaphore for signalling a new frame is ready now
    beginSemaphore = spManager->getSemaphore();

    return true;
}

void VkDriver::shutdown()
{
    vmaDestroyAllocator(vmaAlloc);
}

// =========== functions for buffer/texture creation ================

void VkDriver::addUbo(const Util::String& id, const size_t size, VkBufferUsageFlags usage)
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
}

void VkDriver::addUboAndUpdateDescr(
    const Util::String& id, const size_t size, VkBufferUsageFlags usage)
{
    addUbo(id, size, usage);

    // also update all descriptors that have this id - we haven't added data to the buffer yet but
    // the descriptor sets are only interested in the address and size of the buffer which is known
    // now
    progManager->pushBufferDescrUpdate(id, buffers[{id.c_str()}]);
}

void VkDriver::add2DTexture(
    const Util::String& id,
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    vk::ImageUsageFlags usageFlags)
{
    // for textures, we expect the ids to be unique.
    auto iter = textures.find({id.c_str()});
    assert(iter == textures.end());

    Texture tex;
    tex.create2dTex(*this, format, width, height, mipLevels, usageFlags);
    VkHash::ResourceIdKey key {id.c_str()};
    textures.emplace(key, std::move(tex));
}

void VkDriver::add2DTextureAndPushDecr(
    const Util::String& id,
    vk::Format format,
    const uint32_t width,
    const uint32_t height,
    const uint8_t mipLevels,
    vk::ImageUsageFlags usageFlags)
{
    add2DTexture(id, format, width, height, mipLevels, usageFlags);

    Texture& tex = textures[{id.c_str()}];
    // update the image descriptor set
    progManager->pushImageDescrUpdate(id, tex);
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
    context.getDevice().acquireNextImageKHR(
        swapchain.get(), std::numeric_limits<uint64_t>::max(), beginSemaphore, {}, &imageIndex);
}

void VkDriver::endFrame(Swapchain& swapchain)
{
    // submit all of the cmd buffers - it's supposedly a good idea to keep the number of cmd buffers
    // to a min, as this can impact on performance(?)
    cbManager->submitFrame(swapchain, imageIndex, beginSemaphore);
}


} // namespace VulkanAPI
