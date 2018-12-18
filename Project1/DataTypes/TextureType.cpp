#include "TextureType.h"

#include "stb_image.h"

#include <string>

MappedTexture::MappedTexture()
{
}

bool MappedTexture::loadPngTexture(int size, const unsigned char* imageData)
{
	int w, h, c;

	const stbi_uc* buffer = stbi_load_from_memory(imageData, size, &w, &h, &c, 4);
	if (!buffer) {
		return false;
	}

	width = w;
	height = h;
	numComponents = c;

	// copy image to local store
	memcpy(bin, buffer, width * height * 4);

	stbi_image_free((void*)buffer);

	// setup some vulkan info now to save time now
	vkImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	vkImageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;			// this will need to be looked at - might be using UNORM
	vkImageInfo.imageType = VK_IMAGE_TYPE_2D;
	vkImageInfo.extent.width = width;
	vkImageInfo.extent.height = height;
	vkImageInfo.extent.depth = 1;
	vkImageInfo.mipLevels = 1;		// assuming one here for binary tex files
	vkImageInfo.arrayLayers = 1;
	vkImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	vkImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vkImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	vkImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	vkImageInfo.flags = 0;
}