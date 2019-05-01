#pragma once

#include "Vulkan/Common.h"

namespace OmegaEngine
{

	class MappedTexture
	{

	public:

		MappedTexture();
		~MappedTexture();

		bool map_texture(uint8_t* data, uint32_t w, uint32_t h, uint32_t face_count, uint32_t arrays, uint32_t mips, uint32_t size);
		bool map_texture(uint32_t w, uint32_t h, uint32_t comp, uint8_t* imageData, bool createMipMaps = false);
		bool create_empty_texture(uint32_t w, uint32_t h, bool setToBlack);
		
		uint32_t size() const
		{
			return width * height * 4;		// must be rgba
		}

		void* data()
		{
			return bin;
		}

		uint32_t mipmapCount() const
		{
			return mip_levels;
		}

		uint32_t tex_width() const
		{
			return width;
		}

		uint32_t tex_height() const
		{
			return height;
		}

		vk::Format& get_format()
		{
			return tex_format;
		}

		void set_format(vk::Format format)
		{
			tex_format = format;
		}

		const char* get_name()
		{
			return name;
		}

		void set_name(const char* name)
		{
			this->name = name;
		}

	private:

		// This could probably be public but I am paranoid about unwanted chnages to texture data!
		// info from gltf
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t mip_levels = 1;
		uint32_t array_count = 1;
		uint32_t faces = 1;
		uint32_t total_size = 0;

		const char* name = nullptr;

		// the texture binary
		uint8_t* bin = nullptr;

		// vulkan info that is associated with this texture
		vk::Format tex_format;
	};

}

