#include "TextureType.h"
#include "Utility/logger.h"

#include <string>
#include <algorithm>

namespace OmegaEngine
{
	MappedTexture::MappedTexture()
	{
	}

	MappedTexture::~MappedTexture()
	{
		if (bin) 
		{
			delete[] bin;
		}
	}

	MappedTexture::MappedTexture(MappedTexture&& other) :
		bin(nullptr)
	{
		width = other.width;
		height = other.height;
		mip_levels = other.mip_levels;
		faces = other.faces;
		array_count = other.array_count;
		total_size = other.total_size;
		tex_format = other.tex_format;
		bin = other.bin;

		other.width = 0;
		other.height = 0;
		other.mip_levels = 0;
		other.faces = 0;
		other.array_count = 0;
		other.total_size = 0;
		other.bin = nullptr;
	}

	MappedTexture& MappedTexture::operator=(MappedTexture&& other)
	{
		if (this != &other)
		{
			delete[] bin;
			bin = other.bin;
			width = other.width;
			height = other.height;
			mip_levels = other.mip_levels;
			faces = other.faces;
			array_count = other.array_count;
			total_size = other.total_size;
			tex_format = other.tex_format;

			other.width = 0;
			other.height = 0;
			other.mip_levels = 0;
			other.faces = 0;
			other.array_count = 0;
			other.total_size = 0;
			other.bin = nullptr;
		}
		return *this;
	}

	bool MappedTexture::map_texture(uint8_t* data, uint32_t w, uint32_t h, uint32_t _faces, uint32_t arrays, uint32_t mips, uint32_t size)
	{
		width = w;
		height = h;
		mip_levels = mips;
		faces = _faces;
		array_count = arrays;
		total_size = size;
		tex_format = vk::Format::eR8G8B8A8Unorm;	// an assumption made here - should make this more flexible!

		bin = new uint8_t[total_size];
		if (!bin)
		{
			LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
			return false;
		}

		memcpy(bin, data, total_size);
		return true;
	}

	bool MappedTexture::map_texture(uint32_t w, uint32_t h, uint32_t comp, uint8_t* imageData, bool createMipMaps)
	{
		if (comp == 3) 
		{
			LOGGER_INFO("Unable to map image. Only four channels supported at the moment.");
			return false;
		}

		width = w;
		height = h;
		mip_levels = 1;
		tex_format = vk::Format::eR8G8B8A8Unorm;

		// if using jpg, png, etc. and mip-maps are required, then calculate the max number based on image size
		if (createMipMaps) 
		{
			mip_levels = static_cast<uint8_t>(std::floor(std::log2(std::max(w, h))) + 1.0);
		}
		
		// copy image to local store
		uint32_t image_size = width * height * comp;	// assuming 4 channels here
		bin = new uint8_t[image_size];

		if (!bin) 
		{
			LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
			return false;
		}

		memcpy(bin, imageData, image_size);
		return true;
	}

	bool MappedTexture::create_empty_texture(uint32_t w, uint32_t h, bool setToBlack)
	{
		width = w;
		height = h;
		mip_levels = 1;
		tex_format = vk::Format::eR8G8B8A8Unorm;

		uint32_t image_size = width * height * 4;
		bin = new uint8_t[image_size];

		if (!bin) 
		{
			LOGGER_INFO("Error whilst allocationg memory for empty texture. Out of memory?");
			return false;
		}

		// fill with zeros if required
		if (setToBlack) 
		{
			memset(bin, 0, image_size);
		}

		return true;
	}
}