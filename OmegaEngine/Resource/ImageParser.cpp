#include "ImageParser.h"

namespace ImageUtility
{

KtxReader::KtxReader()
{
}

KtxReader::~KtxReader()
{
	// value-key pair delete
	if (valuePairData)
	{
		delete[] valuePairData;
	}
}

vk::Format KtxReader::convertGlToVkFormat(uint32_t internalFormat)
{
	vk::Format format;
	switch (internalFormat)
	{
	case 15:    // RGB8_ETC2
		format = vk::Format::eEtc2R8G8B8A8UnormBlock;
		break;
	case 16:
		format = vk::Format::eEtc2R8G8B8SrgbBlock;
		break;
	case 19:
		format = vk::Format::eEtc2R8G8B8A8UnormBlock;
		break;
	case 20:
		format = vk::Format::eEtc2R8G8B8A8SrgbBlock;
		break;
	case 32849:
		format = vk::Format::eR8G8B8Unorm;
		break;
	case 32856:
		format = vk::Format::eR8G8B8A8Unorm;
		break;
	default:
		fprintf(stderr, "Undefined glFormat specified.");
	}

	return format;
}

bool KtxReader::open(const char* filename, size_t& fileSize)
{
	FILE* file = fopen(filename, "rb");
	if (!file)
	{
		return false;
	}

	// get the file size
	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	rewind(file);

	// load binary into vector for parsing
	fileBuffer.resize(fileSize);
	size_t readSize = fread(fileBuffer.data(), sizeof(uint8_t), fileSize, file);
	if (readSize != fileSize)
	{
		return false;
	}
	fclose(file);

	return true;
}

bool KtxReader::save(const char* filename, std::vector<uint8_t>& data)
{
	FILE* file = fopen(filename, "wb");
	if (!file)
	{
		fprintf(stderr, "Error creating file %s.\n", filename);
		return false;
	}

	size_t write_size = fwrite(data.data(), sizeof(uint8_t), data.size(), file);
	if (write_size != data.size())
	{
		fprintf(stderr, "Error writing to file %s - incomplete.\n", filename);
		return false;
	}

	fclose(file);
	return true;
}

bool KtxReader::parse(const size_t fileSize)
{
	std::vector<uint8_t>::iterator bufferPos = fileBuffer.begin();

	// to calculate the size of the image data, keep a running tally of the header size,etc.
	uint32_t data_offset = 0;

	// check for the correct header
	if (memcmp(&fileIdentifier, &(*bufferPos), sizeof(fileIdentifier)))
	{
		fprintf(stderr, "File header not recognised. Incorrect file format.");
		return false;
	}
	bufferPos += sizeof(fileIdentifier);
	data_offset += sizeof(fileIdentifier);

	// get all the important info from the file header
	memcpy(&header, &(*bufferPos), sizeof(KtxHeaderV1));
	bufferPos += sizeof(KtxHeaderV1);
	data_offset += sizeof(KtxHeaderV1);

	// according to the spec, if undefined format then we can't continue
	if (header.glFormat == 0)
	{
		fprintf(stderr, "glFormat is undefined. Unable to continue parsing file.");
		return false;
	}

	vk_format = convertGlToVkFormat(header.glInternalFormat);

	// if mip map level is zero - adjust to one for later
	if (!header.numberOfMipmapLevels)
	{
		header.numberOfMipmapLevels = 1;
	}

	// 3d images not supported yet
	if (header.pixelDepth > 0)
	{
		fprintf(stderr, "Using a .ktx file with depth > 1; 3D images are not yet supported.");
		return false;
	}
	else
	{
		header.pixelDepth = 1;
	}

	// and array elements
	if (!header.numberOfArrayElements)
	{
		header.numberOfArrayElements = 1;
	}

	// key value pair data - not used
	uint32_t valuePairSize = 0;
	memcpy(&valuePairSize, &(*bufferPos), sizeof(uint32_t));
	bufferPos += sizeof(uint32_t);
	data_offset += sizeof(uint32_t);

	// get the vlaue-pair data
	valuePairData = new uint8_t[valuePairSize];
	memcpy(valuePairData, &(*bufferPos), valuePairSize);
	bufferPos += valuePairSize;
	data_offset += valuePairSize;

	// deal with padding if any
	uint32_t paddingSize = 3 - ((valuePairSize + 3) % 4);
	if (paddingSize > 0)
	{
		bufferPos += paddingSize;
	}

	// the data also contains the size of the image at each mip level, so account for this in the data offset
	data_offset += sizeof(uint32_t) * header.numberOfMipmapLevels;

	// store the data in one big blob
	imageOutput.data = new uint8_t[fileSize - data_offset];
	uint8_t* dataPtr = imageOutput.data;

	// fill out what we can of the output container
	imageOutput.arrayCount = header.numberOfArrayElements;
	imageOutput.faceCount = header.numberOffaceCount;
	imageOutput.mipLevels = header.numberOfMipmapLevels;
	imageOutput.width = header.pixelWidth;
	imageOutput.height = header.pixelHeight;
	imageOutput.totalSize =
	    (header.pixelWidth * header.pixelHeight * 4 * header.numberOfMipmapLevels * header.numberOffaceCount) *
	    header.numberOfArrayElements;

	// now for the actual images
	for (uint32_t mips = 0; mips < header.numberOfMipmapLevels; ++mips)
	{
		uint32_t mip_size;
		imageOutput.mip_sizes.push_back(mip_size);

		memcpy(&mip_size, &(*bufferPos), sizeof(uint32_t));
		bufferPos += sizeof(uint32_t);

		for (uint32_t element = 0; element < header.numberOfArrayElements; ++element)
		{
			for (uint32_t faceCount = 0; faceCount < header.numberOffaceCount; ++faceCount)
			{
				// assuming pixel depth of one at the mo...
				for (uint32_t depth = 0; depth < header.pixelDepth; ++depth)
				{
					memcpy(dataPtr, &(*bufferPos), mip_size);
					dataPtr += mip_size;
					bufferPos += mip_size;
				}
			}
		}
	}

	return true;
}

std::vector<uint8_t> KtxReader::generate(std::vector<uint8_t>& data, uint32_t width, uint32_t height,
                                         uint32_t arrayCount, uint32_t faceCount, uint32_t mipLevels)
{
	std::vector<uint8_t> output_data;

	// first the file identifier motif
	insert_binary(fileIdentifier, output_data);

	// then the data header
	KtxHeaderV1 header;
	header.endianness = 0;
	header.glType = 0;    // openGl not supported
	header.glTypeSize = 0;
	header.glFormat = 0;
	header.glInternalFormat = 0;
	header.glBaseInternalFormat = 0;
	header.pixelWidth = width;
	header.pixelHeight = height;
	header.pixelDepth = 1;    // must be 1
	header.numberOfArrayElements = arrayCount;
	header.numberOffaceCount = faceCount;
	header.numberOfMipmapLevels = mipLevels;
	insert_binary(header, output_data);

	// key value pair - not using this but set it to be 32bytes
	// this means that we don't need any padding after this struct
	std::array<uint8_t, 32> value_pair;
	insert_binary(value_pair.size(), output_data);
	insert_binary(value_pair.data(), output_data);

	uint32_t data_index = 0;

	// now for the actual images
	for (uint32_t mips = 0; mips < mipLevels; ++mips)
	{
		uint32_t mip_size = width * height * byteAlignment;
		insert_binary(mip_size, output_data);

		for (uint32_t element = 0; element < arrayCount; ++element)
		{
			uint32_t element_size = mip_size * (element * faceCount);

			for (uint32_t face = 0; face < faceCount; ++face)
			{
				// assuming pixel depth of one at the mo...
				for (uint32_t depth = 0; depth < 1; ++depth)
				{
					data_index = element_size + (mip_size * face);
					auto offset = output_data.begin() + data_index;
					std::copy(offset, offset + mip_size, std::back_inserter(output_data));
				}
			}
		}
	}

	return output_data;
}

bool KtxReader::loadFile(const char* filename)
{
	if (!filename)
	{
		// if filename is nullptr, don't error out
		return true;
	}

	size_t fileSize = 0;
	if (!open(filename, fileSize))
	{
		fprintf(stderr, "Unable to open .ktx file: %s.", filename);
		return false;
	}

	if (!parse(fileSize))
	{
		return false;
	}

	return true;
}

bool KtxReader::saveFile(const char* filename, std::vector<uint8_t>& data, uint32_t mipLevels, uint32_t arrayCount,
                         uint32_t num_faceCount, uint32_t width, uint32_t height)
{
	if (!filename)
	{
		fprintf(stderr, "No filename specified.\n");
		return false;
	}

	std::vector<uint8_t> output = generate(data, width, height, arrayCount, num_faceCount, mipLevels);
	if (output.empty())
	{
		fprintf(stderr, "Error whilst generating ktx file binary.\n");
		return false;
	}

	if (!save(filename, output))
	{
		return false;
	}

	return true;
}

}    // namespace ImageUtility