#pragma once

#include "volk.h"

namespace OmegaEngine
{

	class MappedTexture
	{

	public:

		MappedTexture();

		bool loadPngTexture(int size, const unsigned char* imageData);

		int size() const
		{
			return width * height * numComponents;
		}

		void* data()
		{
			return bin;
		}

		int mipmapCount() const
		{
			return numMipMaps;
		}

		int tex_width() const
		{
			return width;
		}

		int tex_height() const
		{
			return height;
		}

		int tex_layers() const
		{
			return numComponents;
		}

		vk::Format& format()
		{
			return tex_format;
		}

	private:

		// This could probably be public but I am paranoid about unwanted chnages to texture data!
		// info from gltf
		int width;
		int height;
		int numComponents;
		int numMipMaps;

		// the texture binary
		unsigned char* bin;

		// vulkan info that is associated with this texture
		vk::Format tex_format;
	};

}

