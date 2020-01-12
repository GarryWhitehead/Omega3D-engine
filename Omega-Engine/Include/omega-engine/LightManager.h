#ifndef LIGHTMANAGER_HPP
#define LIGHTMANAGER_HPP

#include "utility/Compiler.h"

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{
class Engine;

enum class LightType
{
    Spot,
    Point,
    Directional,
    None
};

class OE_PUBLIC LightManager
{

public:
    
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
        
        friend class LightManager;
        
    private:
        
        LightType type = LightType::None;
        OEMaths::vec3f pos;
        OEMaths::colour3 col = OEMaths::colour3{1.0f};
        float fov = 90.0f;
        float intensity = 1000.0f;
        float fallout = 10.0f;
        float radius = 20.0f;
        float scale = 1.0f;
        float offset = 0.0f;
    };
  
    LightManager() = default;
    
private:
    
};

}

#endif /* LIGHTMANAGER_HPP */
