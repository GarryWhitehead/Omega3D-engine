#pragma once

#include "ImageUtils/ImageLoader.h"
#include "ImageUtils/KtxParser.h"
#include "utility/CString.h"

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
    bool mapTexture(
        uint8_t* data,
        uint32_t w,
        uint32_t h,
        uint32_t faceCount,
        uint32_t arrays,
        uint32_t mips,
        uint32_t size,
        const ImageFormat format);

    /**
     * @brief Loads a image file and grabs all the required elements.
     * Checks the filename extension and calls the appropiate parser.
     * At present .ktx, .png and .jpg images are supported.
     */
    bool load(Util::String filename);

    void setDirectory(Util::String dir);

    /**
     * @brief A simple check to determine if the texture is a cube map.
     */
    bool isCubeMap() const;

    uint8_t* getBuffer();
    Util::String& getName();
    uint32_t getWidth() const;
    uint32_t getHeight() const;
    ImageFormat getFormat() const;
    uint32_t getMipLevelCount() const;
    uint32_t getFaceCount() const;

private:
    // dimensions of the image
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    uint32_t arrayCount = 1;
    uint32_t faceCount = 1;
    uint32_t totalSize = 0;

    Util::String name;

    // the mapped texture binary
    uint8_t* buffer = nullptr;

    // vulkan info that is associated with this texture
    ImageFormat format = ImageFormat::Undefined;

    Util::String dirPath;
};

} // namespace OmegaEngine
