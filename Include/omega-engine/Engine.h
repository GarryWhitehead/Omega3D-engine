#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "omega-engine/Scene.h"
#include "omega-engine/World.h"

// forward declerations
namespace VulkanAPI
{
class Swapchain;
class VkDriver;
}

namespace OmegaEngine
{
// forward declerations
class AnimationManager;
class CameraManager;
class LightManager;
class RenderableManager;
class TransformManager;
class Renderer;
class EngineConfig;
struct NativeWindowWrapper;

class Engine
{
public:
        
    Engine() = default;
    ~Engine() = default;

    /**
    * @brief Initialises a new vulkan context. This creates a new instance and
    * prepares the physical and abstract device and associated queues
    * Note: Only one vulkan device is allowed. Multiple devices supporting multi-gpu
    * setups is not yet supported
    */
    bool init(NativeWindowWrapper& window);

    /**
    * @brief This creates a new swapchain instance based upon the platform-specific
    * ntaive window pointer created by the application
    */
    VulkanAPI::Swapchain createSwapchain();

    /**
    * @brief Creates a new renderer instance based on the user specified swapchain and scene
    */
    Renderer* createRenderer(VulkanAPI::Swapchain& swapchain, Scene* scene);

    /**
    * @ brief Creates a new world object. This object is stored by the engine allowing
    * multiple worlds to created if desired.
    * @param name A string name used as a identifier for this world
    * @return Returns a pointer to the newly created world
    */
    World* createWorld(Util::String name);

    /// returns the current vulkan context
    VulkanAPI::VkDriver& getVkDriver();

    AnimationManager& getAnimManager();
    CameraManager& getCameraManager();
    LightManager& getLightManager();
    RenderableManager& getRendManager();
    TransformManager& getTransManager();

private:
    
};

}

#endif /* ENGINE_HPP */
