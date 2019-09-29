#pragma once

#include "Core/Omega_Config.h"

#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"

#include "utility/String.h"

#include <memory>
#include <vector>

// forward declerations
struct NativeWindowWrapper;

namespace OmegaEngine
{
// forward declerations
class World;

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

	bool initDevice(NativeWindowWrapper& window);

	World* createWorld(Util::String name);

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
	VulkanAPI::VkContext context;

	// the swap chain to be used by all worlds.
	// This could at some point become a user dedfined protocol
	// and allow more than swapchain to be created - but for now
	// will stick with just one
	VulkanAPI::Swapchain swapchain;
};

}    // namespace OmegaEngine
