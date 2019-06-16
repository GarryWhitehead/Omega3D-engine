#pragma once

#include "VulkanAPI/Common.h"

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
					data = nullptr;
				}
			}

			ImageOutput(const ImageOutput& other)
			{
				width = other.width;
				height = other.height;
				byteAlignment = other.byteAlignment;
				totalSize = other.totalSize;
				mipLevels = other.mipLevels;
				arrayCount = other.arrayCount;
				faceCount = other.faceCount;

				data = new uint8_t[totalSize];
				memcpy(data, other.data, totalSize);
			}

			ImageOutput& operator=(const ImageOutput& other)
			{
				if (this != &other)
				{
					if (totalSize != other.totalSize)
					{
						delete[] data;
						data = new uint8_t[totalSize];
					}
					memcpy(data, other.data, totalSize);

					width = other.width;
					height = other.height;
					byteAlignment = other.byteAlignment;
					totalSize = other.totalSize;
					mipLevels = other.mipLevels;
					arrayCount = other.arrayCount;
					faceCount = other.faceCount;
				}
			}
			
			// first mip level dimensions
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t byteAlignment = 4;
			uint32_t totalSize = 0;
			uint32_t mipLevels = 0;
			uint32_t arrayCount = 0;
			uint32_t faceCount = 0;

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
			uint32_t numberOffaceCount = 0;
			uint32_t numberOfMipmapLevels = 0;
		} header;

		uint8_t fileIdentifier[12] = {
			0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A
		};

		KtxReader();
		~KtxReader();

		vk::Format convertGlToVkFormat(uint32_t internalFormat);

		bool loadFile(const char* filename);
		bool saveFile(const char* filename, std::vector<uint8_t>& data, uint32_t mipLevels, uint32_t arrayCount, uint32_t num_faceCount, uint32_t width, uint32_t height);

		ImageOutput& getImage_data()
		{
			return imageOutput;
		}

		template <typename T>
		void insert_binary(T data, std::vector<uint8_t>& buffer)
		{
			uint8_t* bin = reinterpret_cast<uint8_t*>(&data);
			std::copy(bin, bin + sizeof(T), std::back_inserter(buffer)); 
		}

	private:

		bool open(const char* filename, size_t& fileSize);
		bool save(const char* filename, std::vector<uint8_t>& output);
		bool parse(const size_t fileSize);
		std::vector<uint8_t> generate(std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t arrayCount, uint32_t faceCount, uint32_t mipLevels);

		// holds the binary file
		std::vector<uint8_t> fileBuffer;

		// vlaue-pair data - not used at present
		uint8_t* valuePairData = nullptr;

		// this will be converted from opengl to vulkan
		vk::Format vk_format;

		// image data for each mip level
		ImageOutput imageOutput;
	};

}


