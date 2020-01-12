#include "Resources.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"


namespace OmegaEngine
{

TextureResource::TextureResource(const uint32_t width, const uint32_t height, const vk::Format format,
                                 const uint8_t level, const uint8_t layers)
    : ResourceBase(ResourceType::Texture)
    , width(width)
    , height(height)
    , layers(layers)
    , level(level)
    , format(format)
{
}

void* TextureResource::bake(VulkanAPI::VkDriver& driver)
{
	texture->create2dTex(driver, format, width, height, level, imageLayout);
	return reinterpret_cast<void*>(texture->getImageView());
}

VulkanAPI::Texture* TextureResource::get()
{
	return texture.get();
}

bool TextureResource::isDepthFormat()
{
    // TODO!
	return true;
}

bool TextureResource::isColourFormat()
{
	// TODO!
	return true;
}

bool TextureResource::isStencilFormat()
{
	// TODO!
	return true;
}

// =============================================================================

void* AttachmentInfo::bake(VulkanAPI::VkDriver& driver, RenderGraph& rGraph)
{
	ResourceBase* base = rGraph.getResource(resource);
	void* data = base->bake(driver);
	return data;
}

}    // namespace OmegaEngine
