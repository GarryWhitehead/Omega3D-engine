#pragma once

#include "Engine/Omega_Config.h"
#include "Vulkan/Device.h"

#include <vector>
#include <memory>
#include <string>
#include <chrono>

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

		World* createWorld(std::string filename, std::string name);
		World* createWorld(std::string name);

		void createWindow(std::string win_title);
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
		std::vector<std::unique_ptr<World> > worlds;
		std::string currentWorld;		

		// a list of all grpahics devices that are available
		std::vector<std::unique_ptr<VulkanAPI::Device>> vkDevices;
		uint32_t currentVkDevice = 0;
	};

}

