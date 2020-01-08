#ifndef LIGHTMANAGER_HPP
#define LIGHTMANAGER_HPP

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{
class Engine;

class LightInstance
{
public:
    
    LightInstance& setType(LightType lt);
    LightInstance& setPosition(const OEMaths::vec3f p);
    LightInstance& setColour(const OEMaths::colour3 c);
    LightInstance& setFov(float f);
    LightInstance& setIntensity(float i);
    LightInstance& setFallout(float fo);
    LightInstance& setRadius(float r);
    
    void create(Engine& engine);
    
private:

};

}

#endif /* LIGHTMANAGER_HPP */
