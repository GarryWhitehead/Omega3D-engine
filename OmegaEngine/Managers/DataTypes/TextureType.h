#pragma once

#include "Vulkan/Common.h"

namespace OmegaEngine
{

	class MappedTexture
	{

	public:

		MappedTexture();
		~MappedTexture();

		MappedTexture(const MappedTexture&) = delete;
		MappedTexture& operator=(const MappedTexture&) = delete;
		MappedTexture(MappedTexture&& other);
		MappedTexture& operator=(MappedTexture&& other);

		bool mapTexture(uint8_t* data, uint32_t w, uint32_t h, uint32_t faceCount, uint32_t arrays, uint32_t mips, uint32_t size);
		bool mapTexture(uint32_t w, uint32_t h, uint32_t comp, uint8_t* imageData, bool createMipMaps = false);
		bool createEmptyTexture(uint32_t w, uint32_t h, bool setToBlack);
		
		uint32_t getImageSize() const
		{
			return width * height * 4;	// must be rgba
		}

		uint32_t getSize() const
		{
			return (width * height * 4 * faceCount) * arrayCount;		
		}

		void* data()
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

		vk::Format& getFormat()
		{
			return format;
		}

		void setFormat(vk::Format format)
		{
			format = format;
		}

		const char* getName()
		{
			return name;
		}

		void setName(const char* name)
		{
			this->name = name;
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

		const char* name = nullptr;

		// the texture binary
		uint8_t* bin = nullptr;

		// vulkan info that is associated with this texture
		vk::Format format;
	};

}

