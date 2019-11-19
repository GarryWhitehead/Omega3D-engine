#pragma once

#include "Core/Omega_Config.h"

#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkDriver.h"

#include "utility/String.h"

#include <memory>
#include <vector>

// forward declerations
namespace VulkanAPI
{
class Swapchain;
}

namespace OmegaEngine
{
// forward declerations
class World;
class AnimationManager;
class CameraManager;
class LightManager;
class RenderableManager;
class TransformManager;

struct NativeWindowWrapper;

class Engine
{
public:
	Engine();
	~Engine();

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
	VulkanAPI::Swapchain createSwapchain(NativeWindowWrapper& window);

	/**
	* @brief Creates a new renderer instance based on the user specified swapchain and scene
	*/
	Renderer* createRenderer(Swapchain& swapchain, Scene* scene);

	/**
	* @ brief Creates a new world object. This object is stored by the engine allowing
	* multiple worlds to created if desired.
	* @param name A string name used as a identifier for this world
	* @return Returns a pointer to the newly created world
	*/
	World* createWorld(Util::String name);

	/// returns the current vulkan context
	VulkanAPI::VkDriver& getVkDriver();

	void loadConfigFile();

	void startLoop();

	// =========== manager getters ===================
	AnimationManager& getAnimManager();
	CameraManager& getCameraManager();
	LightManager& getLightManager();
	RenderableManager& getRendManager();
	TransformManager& getTransManager();

private:
	// configuration for the omega engine
	EngineConfig engineConfig;

	// a collection of worlds registered with the engine
	std::vector<World*> worlds;
	Util::String currentWorld;

	// The vulkan devie. Only one device supported at present
	VulkanAPI::VkDriver vkDriver;

	// All the managers that are required to deal with each of the component types
	// managers are under control of the engine as this allows multiple worlds to use the same 
	// resources from the managers
	AnimationManager* animManager = nullptr;
	CameraManager* cameraManager = nullptr;
	LightManager* lightManager = nullptr;
	RenderableManager* rendManager = nullptr;
	TransformManager* transManager = nullptr;
};

}    // namespace OmegaEngine
