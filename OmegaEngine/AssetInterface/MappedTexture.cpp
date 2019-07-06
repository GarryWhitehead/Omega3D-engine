#include "MappedTexture.h"
#include "Utility/logger.h"

#include <algorithm>
#include <string>

namespace OmegaEngine
{
MappedTexture::MappedTexture()
{
}

MappedTexture::MappedTexture(std::string _name)
    : name(_name)
{
}

MappedTexture::~MappedTexture()
{
	if (bin)
	{
		delete[] bin;
	}
}

MappedTexture::MappedTexture(const MappedTexture &other)
{
	width = other.width;
	height = other.height;
	mipLevels = other.mipLevels;
	faceCount = other.faceCount;
	arrayCount = other.arrayCount;
	totalSize = other.totalSize;
	format = other.format;
	
	if (other.bin)
	{
		uint32_t copySize = width * height * 4;
		bin = new uint8_t[copySize];
		memcpy(bin, other.bin, copySize);
	}

}

MappedTexture &MappedTexture::operator=(const MappedTexture &other)
{
	if (this != &other)
	{
		delete[] bin;
		bin = other.bin;
		width = other.width;
		height = other.height;
		mipLevels = other.mipLevels;
		faceCount = other.faceCount;
		arrayCount = other.arrayCount;
		totalSize = other.totalSize;
		format = other.format;

		if (other.bin)
		{
			uint32_t copySize = width * height * 4;
			bin = new uint8_t[copySize];
			memcpy(bin, other.bin, copySize);
		}
	}
	return *this;
}

MappedTexture::MappedTexture(MappedTexture &&other)
    : bin(nullptr)
{
	width = other.width;
	height = other.height;
	mipLevels = other.mipLevels;
	faceCount = other.faceCount;
	arrayCount = other.arrayCount;
	totalSize = other.totalSize;
	format = other.format;
	bin = other.bin;

	other.width = 0;
	other.height = 0;
	other.mipLevels = 0;
	other.faceCount = 0;
	other.arrayCount = 0;
	other.totalSize = 0;
	other.bin = nullptr;
}

MappedTexture &MappedTexture::operator=(MappedTexture &&other)
{
	if (this != &other)
	{
		delete[] bin;
		bin = other.bin;
		width = other.width;
		height = other.height;
		mipLevels = other.mipLevels;
		faceCount = other.faceCount;
		arrayCount = other.arrayCount;
		totalSize = other.totalSize;
		format = other.format;

		other.width = 0;
		other.height = 0;
		other.mipLevels = 0;
		other.faceCount = 0;
		other.arrayCount = 0;
		other.totalSize = 0;
		other.bin = nullptr;
	}
	return *this;
}

bool MappedTexture::mapTexture(uint8_t *data, uint32_t w, uint32_t h, uint32_t _faceCount,
                               uint32_t arrays, uint32_t mips, uint32_t size, TextureFormat format)
{
	width = w;
	height = h;
	mipLevels = mips;
	faceCount = _faceCount;
	arrayCount = arrays;
	totalSize = size;

	// TODO: add other compressed format support based on whether the particular format is supported by the gpu
	// also need to add support for hdr 16bit float textures
	this->format = format;

	bin = new uint8_t[totalSize];
	if (!bin)
	{
		LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
		return false;
	}

	memcpy(bin, data, totalSize);
	return true;
}

bool MappedTexture::mapTexture(uint32_t w, uint32_t h, uint32_t comp, uint8_t *imageData,
                               TextureFormat format, bool createMipMaps)
{
	if (comp == 3)
	{
		LOGGER_INFO("Unable to map image. Only four channels supported at the moment.");
		return false;
	}

	width = w;
	height = h;
	mipLevels = 1;
	this->format = format;

	// if using jpg, png, etc. and mip-maps are required, then calculate the max number based on image size
	if (createMipMaps)
	{
		mipLevels = static_cast<uint8_t>(std::floor(std::log2(std::max(w, h))) + 1.0);
	}

	// copy image to local store
	uint32_t imageSize = width * height * comp; // assuming 4 channels here
	bin = new uint8_t[imageSize];

	if (!bin)
	{
		LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
		return false;
	}

	memcpy(bin, imageData, imageSize);
	return true;
}

bool MappedTexture::createEmptyTexture(uint32_t w, uint32_t h, TextureFormat format,
                                       bool setToBlack)
{
	width = w;
	height = h;
	mipLevels = 1;
	this->format = format;

	uint32_t imageSize = width * height * 4;
	bin = new uint8_t[imageSize];

	if (!bin)
	{
		LOGGER_INFO("Error whilst allocationg memory for empty texture. Out of memory?");
		return false;
	}

	// fill with zeros if required
	if (setToBlack)
	{
		memset(bin, 0, imageSize);
	}

	return true;
}
} // namespace OmegaEngine