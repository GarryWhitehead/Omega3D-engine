#pragma once

#include "Core/Omega_Config.h"

#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkDriver.h"

#include "utility/String.h"

#include <memory>
#include <vector>

namespace OmegaEngine
{
// forward declerations
class World;
struct NativeWindowWrapper;

// current state of the application
class EngineState
{
public:
	EngineState() = default;
	~EngineState()
	{
	}

	bool getIsRunning() const
	{
		return isRunning;
	}

	void setRunning()
	{
		isRunning = true;
	}

	void stop()
	{
		isRunning = false;
	}

private:
	std::chrono::high_resolution_clock::time_point startTime;

	bool isRunning = true;
	bool isPaused = false;
};

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

private:
	// configuration for the omega engine
	EngineConfig engineConfig;

	EngineState programState;

	// a collection of worlds registered with the engine
	std::vector<World*> worlds;
	Util::String currentWorld;

	// The vulkan devie. Only one device supported at present
	VulkanAPI::VkDriver vkDriver;
};

}    // namespace OmegaEngine
