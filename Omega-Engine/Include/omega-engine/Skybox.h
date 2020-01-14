#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "utility/Compiler.h"

namespace OmegaEngine
{
class MappedTexture;

class OE_PUBLIC Skybox
{
public:
        
    Skybox& setCubeMap(MappedTexture* cm);
    Skybox& setBlurFactor(const float bf);
        
protected:
        
    Skybox() = default;

};
   
}

#endif /* SKYBOX_HPP */
