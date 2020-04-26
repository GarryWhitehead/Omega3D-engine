
#ifndef INDIRECT_LIGHTING_H
#define INDIRECT_LIGHTING_H

#include "utility/Compiler.h"

#include "omega-engine/Skybox.h"

namespace OmegaEngine
{

class OE_PUBLIC IndirectLighting
{
public:
    
    /**
     @brief The environment map which will be used to create the irradiance and specular maps. This must be set if not specifying directly the irradiance and specular maps to use through calls to **specularMap** and **irradianceMap**
     */
    void setEnvMap(Skybox* skybox);
    
    /**
     @brief If set, the indirect lighting reflections will be created using the specified map. Note: if this is set, then the environment map set by **setEnvMap** will be ignored.
     */
    void specularMap(MappedTexture* tex);
    
    /**
    @brief If set, the indirect lighting irradiance will be created using the specified map. Note: if this is set, then the environment map set by **setEnvMap** will be ignored.
    */
    void irradianceMap(MappedTexture* tex);
    
protected:
    
    IndirectLighting() = default;
    ~IndirectLighting() = default;
};

}

#endif /* IndirectLighting_h */
