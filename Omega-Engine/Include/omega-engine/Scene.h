#ifndef SCENE_HPP
#define SCENE_HPP

#include "utility/Compiler.h"

#include "omega-engine/Skybox.h"

namespace OmegaEngine
{
class Engine;
class World;
class Camera;
class Skybox;

class OE_PUBLIC Scene
{
public:
    
    Scene() = default;

    // a scene isn't copyable
    Scene(Scene&) = delete;
    Scene& operator=(Scene&) = delete;

    void update();

    void prepare();

    Camera* getCurrentCamera();
    
    bool addSkybox(Skybox* instance);
    
    void addCamera(Camera* camera);

private:
    
};

}

#endif

