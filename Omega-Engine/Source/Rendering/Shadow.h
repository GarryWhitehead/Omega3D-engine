#pragma once

#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

// forward decleartions
class OEScene;
class OEEngine;

class Shadow
{
public:
    
    struct LightPOV
    {
        OEMaths::mat4f mvp;
    };
    
	Shadow(OEEngine& engine, Scene& scene);
	~Shadow();

	void updateBuffer();

private:
    
    OEEngine& engine;
	OEScene& scene;
};

}    // namespace OmegaEngine
