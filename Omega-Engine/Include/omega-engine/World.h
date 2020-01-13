#ifndef WORLD_HPP
#define WORLD_HPP

#include "utility/Compiler.h"

namespace OmegaEngine
{
// forward declerartions
class Scene;
class Skybox;
class ObjectManager;
class Camera;

class OE_PUBLIC World
{
public:
    
    World() = default;
    
    //void update(double time, double dt);

    /**
    * @brief creates a new empty scene. This will be passed to the renderer for presentation.
    * @return A pointer to the newly created scene
    */
    Scene* createScene();

    Skybox* createSkybox();

    ObjectManager* getObjManager();

    Camera* createCamera();

private:
    
    
};

}

#endif /* World_hpp */
