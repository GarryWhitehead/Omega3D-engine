#include "ImageLoader.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

bool load(Util::String filename, int components)
{
	this->buffer = stbi_load(filename.c_str(), &this->width, &this->height, &this->components, components);
	if (!this->buffer)
	{
		LOGGER_ERROR("Unable to open image file %s.\n", filename.c_str());
		return false;
	}

	return true;
}

}    // namespace OmegaEngine
