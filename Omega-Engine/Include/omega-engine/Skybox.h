#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include "utility/Compiler.h"

namespace OmegaEngine
{
class MappedTexture;

class OE_PUBLIC Skybox
{
public:
    
    class Instance
    {
    public:
        
        Instance& setCubeMap(MappedTexture* cm);
        Instance& setBlurFactor(const float bf);
        
        friend class OEScene;
        
    private:
        
        MappedTexture* cubeMap = nullptr;
        float blur = 0.0f;
    };

    Skybox() = default;
    
private:
    
};

}

#endif /* SKYBOX_HPP */
