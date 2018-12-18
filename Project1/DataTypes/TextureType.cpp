#include "TextureType.h"

#include "stb_image.h"

#include <string>

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
	numComponents = c;

	// copy image to local store
	memcpy(bin, buffer, width * height * 4);

	stbi_image_free((void*)buffer);
}