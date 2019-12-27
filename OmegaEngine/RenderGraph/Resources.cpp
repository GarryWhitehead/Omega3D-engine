#include "Resources.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/Image.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkTexture.h"

namespace OmegaEngine
{

TextureResource::TextureResource(const uint32_t width, const uint32_t height, const vk::Format format, const uint8_t level, const uint8_t layers)
    : width(width)
    , height(height)
    , layers(layers)
    , level(level)
    , format(format)
    , ResourceBase(ResourceType::Texture)
{
}

void* TextureResource::bake()
{
    texture->create2dTex(format, width, height, level, imageLayout);
    return reinterpret_cast<void*>(texture->getImageView());
}

VulkanAPI::Texture* TextureResource::get()
{
    return texture.get();
}

// =============================================================================

void* AttachmentInfo::bake(RenderGraph& rGraph)
{
    ResourceBase* base = rGraph.getResource(resource);
    void* data = base->bake();
    return data;
}

}
