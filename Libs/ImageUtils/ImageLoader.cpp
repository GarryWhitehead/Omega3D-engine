#include "ImageLoader.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

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
