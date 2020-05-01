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
#include "utility/BitSetEnum.h"
#include "utility/CString.h"
#include "utility/Compiler.h"

#include <cstddef>
#include <cstdint>

// vulkan forward declerations
namespace VulkanAPI
{
class ImageView;
class Texture;
class VkDriver;
} // namespace VulkanAPI

namespace OmegaEngine
{

// forward declerations
class RenderGraphPass;
class RenderGraph;

using AttachmentHandle = uint64_t;
using ResourceHandle = uint64_t;

struct ResourceBase
{
    enum class ResourceType
    {
        Texture,
        Imported,
        Buffer
    };

    virtual ~ResourceBase() = default;
    ResourceBase(Util::String name, const ResourceType rtype) : name(name), type(rtype)
    {
    }

    Util::String name; // for degugging
    ResourceType type;


    // ==== set by the compiler =====
    // the number of passes this resource is being used as a input
    size_t readCount = 0;

    RenderGraphPass* writer = nullptr;
};

// All the information needed to build a vulkan texture
struct TextureResource : public ResourceBase
{
    TextureResource(
        Util::String name,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const uint8_t mipLevels,
        const uint8_t faceCount,
        const vk::ImageUsageFlags usageBits,
        const VulkanAPI::LoadClearFlags loadOp = VulkanAPI::LoadClearFlags::Clear,
        const VulkanAPI::LoadClearFlags stencilLoadOp = VulkanAPI::LoadClearFlags::Clear);

    VulkanAPI::Texture* bake(VulkanAPI::VkDriver& driver);

    bool isDepthFormat();
    bool isColourFormat();
    bool isStencilFormat();

    // the image information which will be used to create the image view
    uint8_t samples = 1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t faceCount = 1;
    uint8_t mipLevels = 1;

    vk::Format format = vk::Format::eUndefined; //< The format will determine the type of attachment
    vk::ImageUsageFlags imageUsage;
    
    // these are the only clear flags which are exposed to the user at present
    VulkanAPI::LoadClearFlags loadOp;
    VulkanAPI::LoadClearFlags stencilLoadOp;
};

// used for imported texture targets
struct ImportedResource : public ResourceBase
{
    ImportedResource(
        const Util::String& name,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const uint8_t samples,
        VulkanAPI::ImageView& imageView);

    // This is owned elsewhere - for instance the swapchain
    VulkanAPI::ImageView& imageView;

    uint32_t width = 0;
    uint32_t height = 0;
    vk::Format format = vk::Format::eUndefined;
    uint8_t samples = 0;
};

// A buffer resource
struct BufferResource : public ResourceBase
{
    size_t size;
    VulkanAPI::Buffer::Usage usage;
};

struct AttachmentInfo
{
    AttachmentInfo() = default;

    // creates the 'actual' vulkan resource associated with this attachment
    // void* bake(VulkanAPI::VkDriver& driver, RenderGraph& rGraph);

    Util::String name;
    uint8_t samples = 0;

    // a handle to the resource data which is held by the graph
    ResourceHandle resource;
};


} // namespace OmegaEngine
