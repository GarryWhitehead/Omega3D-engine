#include "ImageLoader.h"

#include "utility/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace OmegaEngine
{

ImageLoader::~ImageLoader()
{
	if (buffer)
	{
		stbi_image_free(buffer);
		buffer = nullptr;
	}
}

bool ImageLoader::load(Util::String filename, int pixels)
{
	buffer = stbi_load(filename.c_str(), &width, &height, &components, pixels);
	if (!buffer)
	{
		LOGGER_ERROR("Unable to open image file %s.\n", filename.c_str());
		return false;
	}

	return true;
}

}    // namespace OmegaEngine
