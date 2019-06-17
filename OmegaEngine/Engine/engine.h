#pragma once

#include "Engine/Omega_Config.h"
#include "VulkanAPI/Device.h"

#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>
#include <string>

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
		~EngineState() {}

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

		World* createWorld(const std::string& filename, const std::string& name);
		World* createWorld(const std::string& name);

		void createWindow(const std::string& win_title);
		void loadConfigFile();

		void startLoop();

		// helper functions
		GLFWwindow* getGlfwWindow() 
		{ 
			assert(window != nullptr);
			return window; 
		}

	private:

		// configuration for the omega engine
		EngineConfig engineConfig;

		EngineState programState;

		// glfw stuff
		GLFWwindow* window = nullptr;
		GLFWmonitor* monitor = nullptr;
		const GLFWvidmode* vmode;
		
		// all keyboard, mouse, gamepad, ect. inputs dealt with here
		std::unique_ptr<InputManager> inputManager;

		// windw details set on init
		uint32_t windowWidth = 0;
		uint32_t windowHeight = 0;
		std::string windowTitle;

		// a collection of worlds registered with the engine
		std::unordered_map<std::string, std::unique_ptr<World> > worlds;
		std::string currentWorld;		

		// a list of all grpahics devices that are available
		std::vector<std::unique_ptr<VulkanAPI::Device>> vkDevices;
		uint32_t currentVkDevice = 0;
	};

}

