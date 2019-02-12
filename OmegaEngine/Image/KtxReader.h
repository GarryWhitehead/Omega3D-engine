#pragma once

#include "Vulkan/Common.h"

#include <cstdint>
#include <vector>

class KtxReader
{
public:

	// assuming that compressed and uncompressed images are 4byte aligned
	const uint32_t byteAlignment = 4;

	struct ImageData
	{
		std::vector<std::vector<uint8_t*> > data;   // for each layer -> face -> image
		uint32_t size;
		uint8_t mipLevel;
	};

	struct KtxHeaderV1
	{
		uint32_t endianness;
		uint32_t glType;                    // openGl not supported
		uint32_t glTypeSize;
		uint32_t glFormat;
		uint32_t glInternalFormat;
		uint32_t glBaseInternalFormat;
		uint32_t pixelWidth;
		uint32_t pixelHeight;
		uint32_t pixelDepth;
		uint32_t numberOfArrayElements;
		uint32_t numberOfFaces;
		uint32_t numberOfMipmapLevels;
	} header;

	uint8_t fileIdentifier[12] = {
		0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
	};

	KtxReader();
	~KtxReader();

	vk::Format convertGlToVkFormat(uint32_t internalFormat);

	bool loadFile(const char* filename);

private:

	bool open(const char* filename);
	bool parse();

	// holds the binary file
	std::vector<uint8_t> fileBuffer;

	// vlaue-pair data - not used at present
	uint8_t* valuePairData = nullptr;

	// this will be converted from opengl to vulkan
	vk::Format vk_format;

	// image data for each mip level
	std::vector<ImageData> imageData;

};


