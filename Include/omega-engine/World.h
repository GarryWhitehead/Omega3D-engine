#ifndef WORLD_HPP
#define WORLD_HPP

namespace OmegaEngine
{
// forward declerartions
class Scene;
class Object;
class ObjectManager;
class Engine;

class World
{
public:
    
    World(Engine& engine, VulkanAPI::VkDriver& driver);
    ~World();

    World() = delete;
    
    void update(double time, double dt);

    /**
    * @brief creates a new empty scene. This will be passed to the renderer for presentation.
    * @return A pointer to the newly created scene
    */
    Scene* createScene();

    ObjectManager& getObjManager();

private:
    
};

}

#endif /* World_hpp */
