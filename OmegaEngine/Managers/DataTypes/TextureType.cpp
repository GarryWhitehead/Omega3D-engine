#include "TextureType.h"
#include "Utility/logger.h"

#include <string>
#include <algorithm>

namespace OmegaEngine
{
	MappedTexture::MappedTexture()
	{
	}

	bool MappedTexture::map_texture(uint32_t w, uint32_t h, uint32_t comp, const unsigned char* imageData)
	{
		if (comp == 3) {
			LOGGER_INFO("Unable to map image. Only four channels supported at the moment.");
			return false;
		}

		width = w;
		height = h;
		mip_levels = static_cast<uint8_t>(std::floor(std::log2(std::max(w, h))) + 1.0);

		// copy image to local store
		uint32_t image_size = width * height * comp;	// assuming 4 channels here
		bin = new unsigned char[image_size];

		if (!bin) {
			LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
			return false;
		}

		memcpy(bin, imageData, image_size);
		return true;
	}

}