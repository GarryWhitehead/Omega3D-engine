#ifndef WORLD_HPP
#define WORLD_HPP

namespace VulkanAPI
{
class VkDriver;
}

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
    
    World() = default;
    
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
