#ifndef WORLD_HPP
#define WORLD_HPP

#include "utility/Compiler.h"

namespace OmegaEngine
{
// forward declerartions
class Scene;
class Skybox;
class Camera;
class Object;

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

    Camera* createCamera();

    Object* createParentObj(const OEMaths::mat4f& world);
    
    Object* createParentObj(const OEMaths::vec3f& trans, const OEMaths::vec3f& scale, const OEMaths::quatf& rot);

    Object* createChildObj(Object* parent);

    void destroyObject(Object* obj);

private:
    
    
};

}

#endif /* World_hpp */
