#pragma once

#include "Engine/Omega_Config.h"
#include "VulkanAPI/Device.h"

#include <vector>
#include <memory>
#include <chrono>
#include <unordered_map>

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

		Engine(const char* win_title, uint32_t width, uint32_t height);
		~Engine();

		World* createWorld(const char* filename, const char* name);
		World* createWorld(const char* name);

		void createWindow(const char* win_title);
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
		char* windowTitle;

		// a collection of worlds registered with the engine
		std::unordered_map<const char*, std::unique_ptr<World> > worlds;
		const char* currentWorld;		

		// a list of all grpahics devices that are available
		std::vector<std::unique_ptr<VulkanAPI::Device>> vkDevices;
		uint32_t currentVkDevice = 0;
	};

}

