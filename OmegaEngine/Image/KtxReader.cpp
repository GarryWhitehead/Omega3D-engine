#include "KtxReader.h"

namespace ImageUtility
{

	KtxReader::KtxReader()
	{
		image = std::make_unique<ImageOutput>();
	}


	KtxReader::~KtxReader()
	{
		// delete image data
		/*for (uint32_t x = 0; x < image->mip_image.size(); ++x) {

			for (uint32_t i = 0; i < image->mip_image[x]->data.size(); ++i) {

				for (uint32_t j = 0; j < image->mip_image[x]->data[i].size(); ++j) {

					if (image->mip_image[x]->data[i][j]) {

						delete[] image->mip_image[x]->data[i][j];
					}
				}
			}
		}*/d

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
		}

		return format;
	}

	bool KtxReader::open(const char* filename)
	{
		FILE* file = fopen(filename, "rb");
		if (!file) 
		{
			return false;
		}

		// get the file size
		fseek(file, 0, SEEK_END);
		uint32_t fileSize = ftell(file);
		rewind(file);

		// load binary into vector for parsing
		fileBuffer.resize(fileSize);
		uint32_t readSize = fread(fileBuffer.data(), sizeof(uint8_t), fileSize, file);
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

		uint32_t write_size = fwrite(data.data(), sizeof(uint8_t), data.size(), file);
		if (write_size != data.size()) 
		{
			fprintf(stderr, "Error writing to file %s - incomplete.\n", filename);
			return false;
		}

		fclose(file);
		return true;
	}

	bool KtxReader::parse()
	{
		std::vector<uint8_t>::iterator bufferPos = fileBuffer.begin();

		// check for the correct header
		if (memcmp(&fileIdentifier, &(*bufferPos), sizeof(fileIdentifier))) 
		{
			fprintf(stderr, "File header not recognised. Incorrect file format.");
			return false;
		}
		bufferPos += sizeof(fileIdentifier);

		// get all the important info from the file header
		memcpy(&header, &(*bufferPos), sizeof(KtxHeaderV1));
		bufferPos += sizeof(KtxHeaderV1);

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

		// get the vlaue-pair data
		valuePairData = new uint8_t[valuePairSize];
		memcpy(valuePairData, &(*bufferPos), valuePairSize);
		bufferPos += valuePairSize;

		// deal with padding if any
		uint32_t paddingSize = 3 - ((valuePairSize + 3) % 4);
		if (paddingSize > 0) 
		{
			bufferPos += paddingSize;
		}

		// now for the actual images
		for (uint32_t mips = 0; mips < header.numberOfMipmapLevels; ++mips) 
		{
			// get the image size
			std::unique_ptr<ImageData> imageInfo = std::make_unique<ImageData>();

			memcpy(&imageInfo->size, &(*bufferPos), sizeof(uint32_t));
			imageInfo->mipLevel = mips;
			bufferPos += sizeof(uint32_t);

			imageInfo->data.resize(header.numberOfArrayElements);
			for (uint32_t element = 0; element < header.numberOfArrayElements; ++element) 
			{
				imageInfo->data[element].resize(header.numberOfFaces);
				for (uint32_t faces = 0; faces < header.numberOfFaces; ++faces) 
				{
					// assuming pixel depth of one at the mo...
					for (uint32_t depth = 0; depth < header.pixelDepth; ++depth) 
					{
						// sort image size for this face
						uint32_t innerSize = header.pixelWidth * header.pixelHeight * byteAlignment;
						if (innerSize != imageInfo->size) 
						{
							fprintf(stderr, "Mismtatch between file image size and calculated size.");
							return false;
						}

						uint8_t* dataPtr = new uint8_t[innerSize];
						imageInfo->data[element][faces] = dataPtr;

						for (uint32_t row = 0; row < header.pixelHeight; ++row) 
						{
							for (uint32_t pixel = 0; pixel < header.pixelWidth; ++pixel) 
							{
 								memcpy(dataPtr, &(*bufferPos), sizeof(byteAlignment));
								dataPtr += byteAlignment;
								bufferPos += byteAlignment;
							}
						}
					}
				}
			}
			
			image->mip_image.emplace_back(std::move(imageInfo));
		}
	}

	bool KtxReader::generate(const char* filename, std::vector<uint8_t>& data)
	{
		std::vector<uint8_t> output_data;
		
		// first the file identifier motif
		insert_binary(fileIdentifier, output_data);

		// then the data header
		KtxHeaderV1 header;
		header.endianness = 0;
		header.glType = 0;                    // openGl not supported
		header.glTypeSize = 0;
		header.glFormat = 0;
		header.glInternalFormat = 0;
		header.glBaseInternalFormat = 0;
		header.pixelWidth = width;
		header.pixelHeight = height;
		header.pixelDepth = 1;	// must be 1
		header.numberOfArrayElements = num_array_elements;
		header.numberOfFaces = num_faces;
		header.numberOfMipmapLevels = mip_levels;
		insert_binary(header, output_data);

		// key value pair - not using this but set it to be 32bytes
		// this means that we don't need any padding after this struct
		std::array<uint8_t, 32> value_pair;
		insert_binary<value_pair.size(), output_data);
		insert_binary<value_pair.data(), output_data);

		uint32_t data_index = 0;

		// now for the actual images
		for (uint32_t mips = 0; mips < mip_levels; ++mips) 
		{
			uint32_t mip_size = width * height * byteAlignment;
			insert_binary(mip_size, output_data);

			for (uint32_t element = 0; element < num_elements; ++element) 
			{
				uint32_t element_size = mip_size * (element * num_faces);

				for (uint32_t faces = 0; faces < num_faces; ++faces) 
				{
					// assuming pixel depth of one at the mo...
					for (uint32_t depth = 0; depth < 1; ++depth) 
					{
						data_index = element_size + (mip_size * faces);
						auto offset = data_output.begin() + data_index;
						std::copy(offset, offset + mip_size, std::back_inserter(output_data));
					}
				}
			}
		}
	}

	bool KtxReader::loadFile(const char* filename)
	{
		if (!filename)
		{
			// if filename is nullptr, don't error out
			return true;
		}
		
		if (!open(filename)) 
		{
			fprintf(stderr, "Unable to open .ktx file: %s.", filename);
			return false;
		}

		if (!parse()) 
		{
			return false;
		}

		return true;
	}

	bool saveFile(const char* filename, uint8_t* data, uint32_t mip_levels, uint32_t num_faces, uint32_t width, uint32_t height)
	{
		if (!filename) 
		{
			fprintf(stderr, "No filename specified.\n", filename);
			return false;
		}

		std::vector<unit8_t> output = generate(data, mip_levels, num_faces, width, height);
		if (!output)
		{
			fprintf(stderr, "Error whilst generating ktx file binary.\n", filename);
			return false;
		}

		if (!save(filename, output))
		{
			return false;
		}
	}

}