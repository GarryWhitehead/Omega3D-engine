#pragma once

#include "Models/ModelImage.h"

#include "utility/String.h"

#include "ImageUtils/KtxParser.h"
#include "ImageUtils/StbLoader.h"

namespace OmegaEngine
{
class Texture
{

public:
	Texture() = default;

	Texture(std::string name);
	~Texture();

	// textures are not copyable but moveable
	Texture(const Texture &other) = delete;
	Texture &operator=(const Texture &other) = delete;
	Texture(Texture &&other) = default;
	Texture &operator=(Texture &&other) = default;

	/**
	* @brief We can just copy a already mapped image to here.
	*/
	bool mapTexture(uint8_t* data, uint32_t w, uint32_t h, uint32_t faceCount, uint32_t arrays, uint32_t mips,
	                uint32_t size, TextureFormat format);

	/**
	* @brief Loads a image file and grabs all the required elements.
	* Checks the filename xtension and calls the appropiate parser.
	* At present .ktx, .png and .jpg images are supported.
	*/
	bool load(Util::String filename);

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
	uint8_t *buffer = nullptr;

	// vulkan info that is associated with this texture
	TextureFormat format;
};

} // namespace OmegaEngine
