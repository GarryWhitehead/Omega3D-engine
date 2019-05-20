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
	class ProgramState
	{
	public:

		ProgramState() = default;
		~ProgramState() {}

		bool is_running() const
		{
			return isRunning;
		}

		void set_running()
		{
			isRunning = true;
		}

		void destroy()
		{
			isRunning = false;
		}

	private:

		// general
		std::chrono::high_resolution_clock::time_point start_time;
		bool isRunning = true;
		bool isPaused = true;
	};

	class Engine
	{
	public:

		Engine(std::string win_title, uint32_t width, uint32_t height);
		~Engine();

		void createWorld(std::string filename, std::string name);
		void createWindow(std::string win_title);
		void loadConfigFile();

		void start_loop();

		// helper functions
		GLFWwindow* get_glfw_window() 
		{ 
			assert(window != nullptr);
			return window; 
		}

	private:

		// configuration for the omega engine
		EngineConfig engine_config;

		std::unique_ptr<ProgramState> program_state;

		// glfw stuff
		GLFWwindow* window = nullptr;
		std::string windowTitle;
		GLFWmonitor* monitor = nullptr;
		const GLFWvidmode* vmode = nullptr;
		
		// all keyboard, mouse, gamepad, ect. inputs dealt with here
		std::unique_ptr<InputManager> inputManager;

		// windw details set on init
		uint32_t windowWidth = 0;
		uint32_t windowHeight = 0;

		// a collection of worlds registered with the engine
		std::vector<std::unique_ptr<World> > worlds;
		uint32_t currentWorldIndex = 0;		

		// a list of all grpahics devices that are available
		std::vector<std::unique_ptr<VulkanAPI::Device>> gfx_devices;
		uint32_t current_gfx_device = 0;
	};

}

