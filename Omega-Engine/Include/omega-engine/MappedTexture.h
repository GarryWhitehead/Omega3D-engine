#ifndef MAPPEDTEXTURE_HPP
#define MAPPEDTEXTURE_HPP

#include "Utility/CString.h"

namespace OmegaEngine
{
    
class MappedTexture
{

public:
    MappedTexture() = default;
    ~MappedTexture();


    /**
    * @brief We can just copy a already mapped image to here.
    */
    bool mapTexture(uint8_t* data, uint32_t w, uint32_t h, uint32_t faceCount, uint32_t arrays, uint32_t mips, uint32_t size, const vk::Format format);

    /**
    * @brief Loads a image file and grabs all the required elements.
    * Checks the filename extension and calls the appropiate parser.
    * At present .ktx, .png and .jpg images are supported.
    */
    bool load(Util::String filename);
    
    /**
     * @brief A simple check to determine if the texture is a cube map.
     */
    bool isCubeMap() const;
    
private:
    
};

}

#endif /*  MAPPEDTEXTURE_HPP */
