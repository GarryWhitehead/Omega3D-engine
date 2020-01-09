#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "omega-engine/Scene.h"
#include "omega-engine/World.h"

#include "utility/CString.h"

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
class LightManager;
class RenderableManager;
class TransformManager;
class Renderer;
class EngineConfig;

class SwapchainHandle
{
public:
    
    SwapchainHandle(uint32_t h) :
        handle(h)
    {}
    
    uint32_t getHandle() const
    {
        return handle;
    }
    
private:
    
    uint32_t handle;
};

/**
     * A wrapper containing all the information needed to create a swapchain.
     */
class WindowInstance
{
public:
    
    void* getNativeWindowPtr();
    
    uint32_t getWidth() const;
    
    uint32_t getHeight() const;
    
private:

};

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
    bool init(WindowInstance& window);

    /**
    * @brief This creates a new swapchain instance based upon the platform-specific
    * ntaive window pointer created by the application
    */
    SwapchainHandle createSwapchain(WindowInstance& window);

    /**
    * @brief Creates a new renderer instance based on the user specified swapchain and scene
    */
    Renderer* createRenderer(WindowInstance& window, SwapchainHandle& handle, Scene* scene);

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
    LightManager& getLightManager();
    RenderableManager& getRendManager();
    TransformManager& getTransManager();

private:
    
};

}

#endif /* ENGINE_HPP */
