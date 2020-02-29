#include "Resources.h"

#include "RenderGraph/RenderGraph.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/utility.h"


namespace OmegaEngine
{

TextureResource::TextureResource(
    Util::String name,
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const uint8_t level,
    const uint8_t layers,
    const vk::ImageUsageFlagBits usageBits)
    : ResourceBase(name, ResourceType::Texture)
    , width(width)
    , height(height)
    , layers(layers)
    , level(level)
    , format(format)
    , imageUsage(usageBits)
{
}

void* TextureResource::bake(VulkanAPI::VkDriver& driver)
{
    driver.add2DTexture(name, format, width, height, level, imageUsage);
    return reinterpret_cast<void*>(texture->getImageView());
}

VulkanAPI::Texture* TextureResource::get()
{
    return texture.get();
}

bool TextureResource::isDepthFormat()
{
    if (VulkanAPI::VkUtil::isDepth(format))
    {
        return true;
    }
    return false;
}

bool TextureResource::isColourFormat()
{
    if (!VulkanAPI::VkUtil::isDepth(format) && !VulkanAPI::VkUtil::isStencil(format))
    {
        return true;
    }
    return false;
}

bool TextureResource::isStencilFormat()
{
    if (VulkanAPI::VkUtil::isStencil(format))
    {
        return true;
    }
    return false;
}

ImportedResource::ImportedResource(
    Util::String name, const uint32_t width, const uint32_t height, VulkanAPI::ImageView* imageView)
    : ResourceBase(name, ResourceType::Imported) ,
    width(width), height(height), imageView(imageView)
{
}

// =============================================================================

void* AttachmentInfo::bake(VulkanAPI::VkDriver& driver, RenderGraph& rGraph)
{
    ResourceBase* base = rGraph.getResource(resource);
    void* data = base->bake(driver);
    return data;
}

} // namespace OmegaEngine
