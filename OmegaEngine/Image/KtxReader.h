#pragma once

#include "Vulkan/Common.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

namespace ImageUtility
{
	
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

		struct ImageOutput
		{
			uint32_t width;
			uint32_t height;

			std::vector<std::unique_ptr<ImageData> > mip_image;
		};

		struct KtxHeaderV1
		{
			uint32_t endianness = 0;
			uint32_t glType = 0;                    // openGl not supported
			uint32_t glTypeSize = 0;
			uint32_t glFormat = 0;
			uint32_t glInternalFormat = 0;
			uint32_t glBaseInternalFormat = 0;
			uint32_t pixelWidth = 0;
			uint32_t pixelHeight = 0;
			uint32_t pixelDepth = 0;
			uint32_t numberOfArrayElements = 0;
			uint32_t numberOfFaces = 0;
			uint32_t numberOfMipmapLevels = 0;
		} header;

		uint8_t fileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};

		KtxReader();
		~KtxReader();

		vk::Format convertGlToVkFormat(uint32_t internalFormat);

		bool loadFile(const char* filename);
		bool saveFile(const char* filename, uint8_t* data, uint32_t mip_levels, uint32_t num_faces, uint32_t width, uint32_t height);

		std::unique_ptr<ImageOutput> get_image_data()
		{
			return std::move(image);
		}

		template <typename T>
		void insert_binary(T data, std::vector<uint8_t>& buffer)
		{
			uint8_t* bin = reinterpret_cast<uint8_t*>(&data);
			std::copy(bin, bin + sizeof(T), std::back_inserter(buffer)); 
		}

	private:

		bool open(const char* filename);
		bool save(const char* filename);
		bool parse();
		bool generate();

		// holds the binary file
		std::vector<uint8_t> fileBuffer;

		// vlaue-pair data - not used at present
		uint8_t* valuePairData = nullptr;

		// this will be converted from opengl to vulkan
		vk::Format vk_format;

		// image data for each mip level
		std::unique_ptr<ImageOutput> image;
	};

}


