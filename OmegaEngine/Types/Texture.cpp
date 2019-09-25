#include "Texture.h"

#include "Utility/logger.h"

#include <algorithm>
#include <string>

namespace OmegaEngine
{

Texture::~Texture()
{
	if (buffer)
	{
		delete[] buffer;
	}
}

bool Texture::mapTexture(uint8_t* data, uint32_t w, uint32_t h, uint32_t faces, uint32_t arrays, uint32_t mips,
                         uint32_t size, TextureFormat format)
{
	width = w;
	height = h;
	mipLevels = mips;
	faceCount = faces;
	arrayCount = arrays;
	totalSize = size;
	assert(totalSize);

	// TODO: add other compressed format support based on whether the particular format is supported by the gpu
	// also need to add support for hdr 16bit float textures
	this->format = format;

	this->buffer = new uint8_t[totalSize];
	if (!this->buffer)
	{
		LOGGER_INFO("Error whilst allocationg memory for texture. Out of memory?");
		return false;
	}

	memcpy(this->buffer, data, totalSize);
	return true;
}

bool Texture::load(Util::String filePath)
{
	// get the extension
	auto split = filePath.split('.');
	Util::String ext = split.back();

	if (split.size() == 1)
	{
		LOGGER_ERROR("File name %s has no extension. Unable to interpret image format.", filePath.c_str());
		return false;
	}

	if (ext.compare("ktx"))
	{
		KtxReader parser;
		if (!parser.loadFile(filePath))
		{
			LOGGER_ERROR("Unable to open ktx image file: %s", filePath.c_str());
			return false;
		}

		KtxReader::ImageOutput* data = parser.getImageData();
		this->width = data->width;
		this->height = data->height;
		this->mipLevels = data->mipLevels;
		this->arrayCount = data->arrayCount;
		this->faceCount = data->faceCount;
		this->totalSize = data->totalSize;
		assert(totalSize);
		assert(data->data);

		// create the buffer and copy the pixels across
		buffer = new uint8_t[totalSize];
		memcpy(buffer, data->data, totalSize);
	}
	else if (ext.compare("png") || ext.compare("jpg"))
	{
		ImageLoader loader;
		if (!loader.load(filePath))
		{
			return false;
		}

		this->width = loader.getWidth();
		this->height = loader.getHeight();

		// calculate total size
		int comp = loader.getComponentCount();
		this->totalSize = width * height * comp;

		// create the buffer and copy the data across
		buffer = new uint8_t[totalSize];
		memcpy(buffer, loader.getData(), totalSize);
	}
	else
	{
		LOGGER_ERROR("Unsupported image extension. File path: %s", filePath.c_str());
		return false;
	}

	return true;
}


}    // namespace OmegaEngine