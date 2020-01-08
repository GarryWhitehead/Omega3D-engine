#pragma once

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

// forward decleartions
class Scene;
class Engine;

class Shadow
{
public:
    
    struct LightPOV
    {
        OEMaths::mat4f mvp;
    };
    
	Shadow(Engine& engine, Scene& scene);
	~Shadow();

	void updateBuffer();

private:
    
    Engine& engine;
	Scene& scene;
};

}    // namespace OmegaEngine
