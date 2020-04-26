#include "Resources.h"

#include "RenderGraph/RenderGraph.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "VulkanAPI/Utility.h"


namespace OmegaEngine
{

TextureResource::TextureResource(
    Util::String name,
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const uint8_t mipLevels,
    const uint8_t faceCount,
    const vk::ImageUsageFlags usageBits)
    : ResourceBase(name, ResourceType::Texture)
    , width(width)
    , height(height)
    , mipLevels(mipLevels)
    , faceCount(faceCount)
    , format(format)
    , imageUsage(usageBits)
{
}

VulkanAPI::Texture* TextureResource::bake(VulkanAPI::VkDriver& driver)
{
    // TODO: need to add support for arrays too
    return driver.findOrCreateTexture2d(name, format, width, height, mipLevels, faceCount, 1, imageUsage);
}

VulkanAPI::Texture* TextureResource::get(VulkanAPI::VkDriver& driver)
{
    return driver.getTexture2D(name);
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
    const Util::String& name, const uint32_t width, const uint32_t height, const vk::Format format,
           const uint8_t samples, VulkanAPI::ImageView& imageView)
    : ResourceBase(name, ResourceType::Imported) ,
    imageView(imageView), width(width), height(height), format(format), samples(samples)
{
}

// =============================================================================

/*void* AttachmentInfo::bake(VulkanAPI::VkDriver& driver, RenderGraph& rGraph)
{
    ResourceBase* base = rGraph.getResource(resource);
    if (base->type == ResourceBase::ResourceType::Texture)
    {
        void* data = base->bake(driver);
        return data;
    }
}*/

} // namespace OmegaEngine
