/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "utility/CString.h"

#include <stdio.h>

#include <cstdint>
#include <vector>

namespace OmegaEngine
{

enum class ImageFormat
{
    Etc_RGBA_UnormBlock,
    Etc_RGBA_SrgbBlock,
    Etc_RGB_UnormBlock,
    Etc_RGB_SrgbBlock,
    RGBA_Unorm,
    RGB_Unorm,
    Undefined
};

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
		uint32_t glType = 0;    // openGl not supported
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

	uint8_t fileIdentifier[12] = { 0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A };

	KtxReader();
	~KtxReader();

	ImageFormat convertGlToImageFormat(uint32_t internalFormat);

	bool loadFile(Util::String filename);
	
    bool saveFile(Util::String filename, uint32_t mipLevels, uint32_t arrayCount,
	              uint32_t num_faceCount, uint32_t width, uint32_t height);

	ImageOutput* getImageData()
	{
		return &imageOutput;
	}

private:

	bool load(Util::String filename, size_t& fileSize);
	
    bool save(Util::String, std::vector<uint8_t>& output);
	
    bool parse(const size_t fileSize);
	
    std::vector<uint8_t> generate(uint32_t width, uint32_t height, uint32_t arrayCount,
	                              uint32_t faceCount, uint32_t mipLevels);

	template <typename T>
	void insert_binary(T data, std::vector<uint8_t>& buffer)
	{
		uint8_t* bin = reinterpret_cast<uint8_t*>(&data);
		std::copy(bin, bin + sizeof(T), std::back_inserter(buffer));
	}

private:
	
	// holds the binary file
	std::vector<uint8_t> fileBuffer;

	// vlaue-pair data - not used at present
	uint8_t* valuePairData = nullptr;

	// this will be converted from opengl to vulkan
	ImageFormat imageFormat;

	// image data for each mip level
	ImageOutput imageOutput;
};

}    // namespace ImageUtility
