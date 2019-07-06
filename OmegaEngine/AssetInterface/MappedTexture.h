#pragma once

#include "Models/ModelImage.h"
#include "VulkanAPI/Common.h"
#include <string>

namespace OmegaEngine
{
class MappedTexture
{

public:
	MappedTexture();
	MappedTexture(std::string name);
	~MappedTexture();

	MappedTexture(const MappedTexture &other);
	MappedTexture &operator=(const MappedTexture &other);
	MappedTexture(MappedTexture &&other);
	MappedTexture &operator=(MappedTexture &&other);

	bool mapTexture(uint8_t *data, uint32_t w, uint32_t h, uint32_t faceCount, uint32_t arrays,
	                uint32_t mips, uint32_t size, TextureFormat format);
	bool mapTexture(uint32_t w, uint32_t h, uint32_t comp, uint8_t *imageData, TextureFormat format,
	                bool createMipMaps = false);
	bool createEmptyTexture(uint32_t w, uint32_t h, TextureFormat format, bool setToBlack);

	uint32_t getImageSize() const
	{
		return width * height * 4; // must be rgba
	}

	uint32_t getSize() const
	{
		return (width * height * 4 * faceCount) * arrayCount;
	}

	void *data()
	{
		return bin;
	}

	uint32_t mipmapCount() const
	{
		return mipLevels;
	}

	uint32_t textureWidth() const
	{
		return width;
	}

	uint32_t textureHeight() const
	{
		return height;
	}

	uint32_t getFaceCount() const
	{
		return faceCount;
	}

	uint32_t getArrayCount() const
	{
		return arrayCount;
	}

	TextureFormat &getFormat()
	{
		return format;
	}

	std::string &getName()
	{
		return name;
	}

private:
	// This could probably be public but I am paranoid about unwanted chnages to texture data!
	// info from gltf
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipLevels = 1;
	uint32_t arrayCount = 1;
	uint32_t faceCount = 1;
	uint32_t totalSize = 0;

	std::string name;

	// the texture binary
	uint8_t *bin = nullptr;

	// vulkan info that is associated with this texture
	TextureFormat format;
};

} // namespace OmegaEngine
