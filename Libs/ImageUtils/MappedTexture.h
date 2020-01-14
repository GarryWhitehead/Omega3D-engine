#pragma once

#include "utility/CString.h"

#include "ImageUtils/ImageLoader.h"
#include "ImageUtils/KtxParser.h"

#include <string>

namespace OmegaEngine
{
class MappedTexture
{

public:
	MappedTexture() = default;
	~MappedTexture();

	// textures are not copyable
	MappedTexture(const MappedTexture& other) = delete;
	MappedTexture& operator=(const MappedTexture& other) = delete;

	/**
	* @brief We can just copy a already mapped image to here.
	*/
	bool mapTexture(uint8_t* data, uint32_t w, uint32_t h, uint32_t faceCount, uint32_t arrays, uint32_t mips,
	                uint32_t size, const ImageFormat format);

	/**
	* @brief Loads a image file and grabs all the required elements.
	* Checks the filename extension and calls the appropiate parser.
	* At present .ktx, .png and .jpg images are supported.
	*/
	bool load(Util::String filename);

	void setDirectory(Util::String dir)
	{
		dirPath = dir;
	}

	/**
     * @brief A simple check to determine if the texture is a cube map.
     */
	bool isCubeMap() const
	{
		return faceCount == 6;
	}

	friend class OERenderableManager;

private:
	// dimensions of the image
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t mipLevels = 1;
	uint32_t arrayCount = 1;
	uint32_t faceCount = 1;
	uint32_t totalSize = 0;

	std::string name;

	// the mapped texture binary
	uint8_t* buffer = nullptr;

	// vulkan info that is associated with this texture
	ImageFormat format;

	Util::String dirPath;
};

}    // namespace OmegaEngine
