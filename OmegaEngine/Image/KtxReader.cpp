#include "KtxReader.h"
#include "Utility/Logger.h"

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
		}*/

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

	bool KtxReader::parse()
	{
		std::vector<uint8_t>::iterator bufferPos = fileBuffer.begin();

		// check for the correct header
		if (memcmp(&fileIdentifier, &(*bufferPos), sizeof(fileIdentifier))) 
		{
			LOGGER_ERROR("File header not recognised. Incorrect file format.");
		}
		bufferPos += sizeof(fileIdentifier);

		// get all the important info from the file header
		memcpy(&header, &(*bufferPos), sizeof(KtxHeaderV1));
		bufferPos += sizeof(KtxHeaderV1);

		// according to the spec, if undefined format then we can't continue
		if (header.glFormat == 0) 
		{
			LOGGER_ERROR("glFormat is undefined. Unable to continue parsing file.");
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
			LOGGER_ERROR("Using a .ktx file with depth > 1; 3D images are not yet supported.");
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
							LOGGER_ERROR("Mismtatch between file image size and calculated size.");
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

	bool KtxReader::loadFile(const char* filename)
	{
		if (!filename)
		{
			// if filename is nullptr, don't error out
			return true;
		}
		
		if (!open(filename)) 
		{
			LOGGER_ERROR("Unable to open .ktx file: %s.", filename);
		}

		if (!parse()) 
		{
			return false;
		}

		return true;
	}

}