#pragma once

#include "omega-engine/Engine.h"

#include "Components/AnimationManager.h"
#include "Components/LightManager.h"
#include "Components/RenderableManager.h"
#include "Components/TransformManager.h"

#include "VulkanAPI/Platform/Surface.h"
#include "VulkanAPI/VkDriver.h"

#include "Scripting/OEConfig.h"

#include "utility/CString.h"

#include <memory>
#include <vector>

namespace OmegaEngine
{
// forward declerations
class OEWorld;;
class Renderer;
class OEScene;
class EngineConfig;
class OEWindowInstance;

class OEEngine : public Engine
{
public:
    
    // for now the default values for engine config are stored here
    static constexpr float Default_ClearVal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    
	OEEngine();
	~OEEngine();

	/**
	* @brief Initialises a new vulkan context. This creates a new instance and 
	* prepares the physical and abstract device and associated queues
	* Note: Only one vulkan device is allowed. Multiple devices supporting multi-gpu
	* setups is not yet supported
	*/
	bool init(OEWindowInstance* window);

	/**
	* @brief This creates a new swapchain instance based upon the platform-specific
	* ntaive window pointer created by the application
	*/
	SwapchainHandle createSwapchain(OEWindowInstance* window);
	
	/**
	* @brief Creates a new renderer instance based on the user specified swapchain and scene
	*/
	Renderer* createRenderer(SwapchainHandle& handle, OEScene* scene);

	/**
	* @ brief Creates a new world object. This object is stored by the engine allowing
	* multiple worlds to created if desired.
	* @param name A string name used as a identifier for this world
	* @return Returns a pointer to the newly created world
	*/
	OEWorld* createWorld(Util::String name);

	/// returns the current vulkan context
	VulkanAPI::VkDriver& getVkDriver();

	// =========== manager getters ===================
	AnimationManager& getAnimManager();
	OELightManager* getLightManager();
	OERenderableManager* getRendManager();
	TransformManager& getTransManager();

private:
	
	// a collection of worlds registered with the engine
	std::vector<OEWorld*> worlds;
	Util::String currentWorld;
    
    // A list of renderers which have been created
    std::vector<Renderer*> renderers;
    
	// The vulkan devie. Only one device supported at present
	VulkanAPI::VkDriver vkDriver;
    
    // the configuration for this engine
    EngineConfig config;
    
    VulkanAPI::Platform::SurfaceWrapper surface;
    
	// keep a list of active swapchains here
	std::vector<std::unique_ptr<VulkanAPI::Swapchain>> swapchains;

	// All the managers that are required to deal with each of the component types
	// managers are under control of the engine as this allows multiple worlds to use the same 
	// resources from the managers
	std::unique_ptr<AnimationManager> animManager;
	std::unique_ptr<OELightManager> lightManager;
	std::unique_ptr<OERenderableManager> rendManager;
	std::unique_ptr<TransformManager> transManager;
};

}    // namespace OmegaEngine
