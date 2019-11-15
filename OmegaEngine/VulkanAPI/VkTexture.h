#pragma once

#include "VulkanAPI/Common.h"

#include <memory>

namespace OmegaEngine
{
enum class TextureFormat;
class MappedTexture;
}    // namespace OmegaEngine

namespace VulkanAPI
{
// forward declerations
class Image;
class ImageView;

class Texture
{

public:
	/**
	* A simple struct for storing all texture info and ease of passing around 
	*/
	struct TextureContext
	{
		vk::Format format;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t faceCount = 1;
		uint32_t arrays = 1;
		uint32_t mipLevels = 1;
	};

	Texture() = default;
	~Texture();

	void init(VkContext& context);

	static vk::Format convertTextureFormatToVulkan(OmegaEngine::TextureFormat format);

	void createEmptyImage(vk::Format, uint32_t width, uint32_t height, uint8_t mipLevels,
	                      vk::ImageUsageFlags usageFlags, uint32_t faces = 1);
	void map(OmegaEngine::MappedTexture& tex);
	void createCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers);
	void createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers);

	vk::ImageView& getImageView();

	TextureContext& getContext();

private:

	VkContext& context;

	// texture info
	TextureContext context;

	std::unique_ptr<Image> image;
	std::unique_ptr<ImageView> imageView;
};

}    // namespace VulkanAPI
