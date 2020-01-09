#ifndef SCENE_HPP
#define SCENE_HPP

#include "omega-engine/Skybox.h"

namespace VulkanAPI
{
class VkDriver;
}

namespace OmegaEngine
{
class Engine;
class World;
class Camera;

class Scene
{
public:

    Scene() = delete;
    
    Scene(World& world, Engine& engine, VulkanAPI::VkDriver& driver);
    ~Scene();

    // a scene isn't copyable
    Scene(Scene&) = delete;
    Scene& operator=(Scene&) = delete;

    void update();

    void prepare();

    Camera* getCurrentCamera();
    
    bool addSkybox(Skybox::Instance& instance);
    
    void addCamera(const Camera& camera);

private:
    
};

}

#endif


