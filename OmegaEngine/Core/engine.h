#pragma once

#include "Core/Omega_Config.h"
#include "VulkanAPI/Device.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// forward declerations
struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace OmegaEngine
{
// forward declerations
class World;
class InputManager;

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
	Engine(std::string win_title, uint32_t width, uint32_t height);
	~Engine();

	World *createWorld(const std::string &filename, const std::string &name);
	World *createWorld(const std::string &name);

	void loadConfigFile();

	void startLoop();

private:
	// configuration for the omega engine
	EngineConfig engineConfig;

	EngineState programState;
	
	// a collection of worlds registered with the engine
	std::unordered_map<std::string, std::unique_ptr<World>> worlds;
	std::string currentWorld;

	// The vulkan devie. Only one device supported at present
	VulkanAPI::Device gfxDevice;
};

} // namespace OmegaEngine
