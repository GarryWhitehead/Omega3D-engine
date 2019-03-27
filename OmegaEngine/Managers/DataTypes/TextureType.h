#pragma once

#include "Vulkan/Common.h"

namespace OmegaEngine
{

	class MappedTexture
	{

	public:

		MappedTexture();

		bool map_texture(uint32_t w, uint32_t h, uint32_t comp, unsigned char* imageData, bool createMipMaps = false);

		int size() const
		{
			return width * height * 4;		// must be rgba
		}

		void* data()
		{
			return bin;
		}

		int mipmapCount() const
		{
			return mip_levels;
		}

		int tex_width() const
		{
			return width;
		}

		int tex_height() const
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
		int width;
		int height;
		int mip_levels;
		const char* name;

		// the texture binary
		unsigned char* bin;

		// vulkan info that is associated with this texture
		vk::Format tex_format;
	};

}

