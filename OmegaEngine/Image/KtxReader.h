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
		constexpr static uint32_t byteAlignment = 4;

		struct ImageOutput
		{
			ImageOutput() = default;
			~ImageOutput() 
			{
				if (data)
				{
					delete[] data;
				}
			}
			
			// first mip level dimensions
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t byte_alignment = 4;
			uint32_t total_size = 0;
			uint32_t mip_levels = 0;
			uint32_t array_count = 0;
			uint32_t faces = 0;

			// total image size of each mip level (width * height * byteAlignment)
			std::vector<uint32_t> mip_sizes;

			uint8_t* data = nullptr;
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
		bool saveFile(const char* filename, std::vector<uint8_t>& data, uint32_t mip_levels, uint32_t array_count, uint32_t num_faces, uint32_t width, uint32_t height);

		ImageOutput& get_image_data()
		{
			return image_output;
		}

		template <typename T>
		void insert_binary(T data, std::vector<uint8_t>& buffer)
		{
			uint8_t* bin = reinterpret_cast<uint8_t*>(&data);
			std::copy(bin, bin + sizeof(T), std::back_inserter(buffer)); 
		}

	private:

		bool open(const char* filename, uint32_t& fileSize);
		bool save(const char* filename, std::vector<uint8_t>& output);
		bool parse(const uint32_t file_size);
		std::vector<uint8_t> generate(std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t array_count, uint32_t faces, uint32_t mip_levels);

		// holds the binary file
		std::vector<uint8_t> fileBuffer;

		// vlaue-pair data - not used at present
		uint8_t* valuePairData = nullptr;

		// this will be converted from opengl to vulkan
		vk::Format vk_format;

		// image data for each mip level
		ImageOutput image_output;
	};

}


