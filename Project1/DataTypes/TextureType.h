#pragma once

#include "volk.h"

class MappedTexture
{
	
public:

	MappedTexture();

	bool loadPngTexture(int size, const unsigned char* imageData);

private:

	// This could probably be public but I am paranoid about unwanted chnages to texture data!
	// info from gltf
	int width;
	int height;
	int numComponents;

	// the texture binary
	unsigned char* bin;

	// vulkan info that is associated with this texture
	VkImageCreateInfo vkImageInfo = {};
};

