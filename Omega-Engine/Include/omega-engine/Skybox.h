#ifndef SKYBOX_HPP
#define SKYBOX_HPP

namespace OmegaEngine
{
class MappedTexture;
    
class SkyboxInstance
{
public:
    
    SkyboxInstance& setCubeMap(MappedTexture* cm);
    
    SkyboxInstance& setBlurFactor(const float bf);
        
private:
    
};

}

#endif /* SKYBOX_HPP */
