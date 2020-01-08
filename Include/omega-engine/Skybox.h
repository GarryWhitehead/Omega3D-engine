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
    
    friend class Scene;
    
private:
    
};

}

#endif /* SKYBOX_HPP */
