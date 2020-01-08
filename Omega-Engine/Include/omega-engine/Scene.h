#ifndef SCENE_HPP
#define SCENE_HPP

namespace OmegaEngine
{

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
    
    bool addSkybox(SkyboxInstance& instance);
    
    void addCamera(const Camera camera);

private:
    
};

}

#endif


