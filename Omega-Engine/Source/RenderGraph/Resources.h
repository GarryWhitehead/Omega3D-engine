#pragma once

#include "VulkanAPI/Buffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/RenderPass.h"
#include "utility/BitsetEnum.h"
#include "utility/CString.h"

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

    Util::String name;
    ResourceType type;

    // ==== set by the compiler =====
    // the number of passes this resource is being used as a input
    size_t readCount = 0;

    RenderGraphPass* writer = nullptr;

    // used by the attachment descriptor
    uint32_t referenceId = 0;
};

// All the information needed to build a vulkan texture
struct TextureResource : public ResourceBase
{
    TextureResource(
        Util::String name,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const uint8_t level,
        const uint8_t layers,
        const vk::ImageUsageFlagBits usageBits);

    void* bake(VulkanAPI::VkDriver& driver);
    VulkanAPI::Texture* get();

    bool isDepthFormat();
    bool isColourFormat();
    bool isStencilFormat();

    // the image information which will be used to create the image view
    uint8_t samples = 1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint8_t layers = 1; //< 3d textures not supported at present
    uint8_t level = 0; //< For mult-sampling. Not used at present

    vk::Format format = vk::Format::eUndefined; //< The format will determine the type of attachment
    vk::ImageUsageFlags imageUsage;

    VulkanAPI::RenderPass::ClearFlags clearFlags;
};

// used for imported texture targets 
struct ImportedResource : public ResourceBase
{
    ImportedResource(
        Util::String name,
        const uint32_t width,
        const uint32_t height,
        VulkanAPI::ImageView* imageView);

    // This is owned elsewhere - for instance the swapchain
    VulkanAPI::ImageView* imageView = nullptr;

    uint32_t width = 0;
    uint32_t height = 0;
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
    void* bake(VulkanAPI::VkDriver& driver, RenderGraph& rGraph);

    Util::String name;
    uint8_t samples = 0;

    // a handle to the resource data which is held by the graph
    ResourceHandle resource;
};


} // namespace OmegaEngine
