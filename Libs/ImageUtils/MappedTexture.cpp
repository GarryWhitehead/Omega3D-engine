#include "MappedTexture.h"

#include "Utility/logger.h"

#include <algorithm>
#include <string>

namespace OmegaEngine
{

MappedTexture::~MappedTexture()
{
	if (buffer)
	{
		delete[] buffer;
	}
}

bool MappedTexture::mapTexture(uint8_t* data, uint32_t w, uint32_t h, uint32_t faces, uint32_t arrays, uint32_t mips, uint32_t size, const ImageFormat format)
{
	width = w;
	height = h;
	mipLevels = mips;
	faceCount = faces;
	arrayCount = arrays;
	totalSize = size;
	assert(totalSize);
    
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

bool MappedTexture::load(Util::String filePath)
{
	// get the extension
	auto split = Util::String::split(filePath, '.');
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
		width = data->width;
		height = data->height;
		mipLevels = data->mipLevels;
		arrayCount = data->arrayCount;
		faceCount = data->faceCount;
		totalSize = data->totalSize;
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

		width = loader.getWidth();
		height = loader.getHeight();

		// calculate total size
		int comp = loader.getComponentCount();
		totalSize = width * height * comp;

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
