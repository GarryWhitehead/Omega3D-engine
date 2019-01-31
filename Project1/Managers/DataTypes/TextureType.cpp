#include "TextureType.h"

#include "stb_image.h"

#include <string>
#include <algorithm>

namespace OmegaEngine
{
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
		mip_levels = static_cast<uint8_t>(std::floor(std::log2(std::max(w, h))) + 1.0);

		// copy image to local store
		memcpy(bin, buffer, width * height * 4);

		stbi_image_free((void*)buffer);

	}

}