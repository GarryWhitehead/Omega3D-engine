#pragma once
#include <vector>
#include <memory>

#include "Engine/Omega_Config.h"
#include "Vulkan/Device.h"

// forward declerations
struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;

namespace OmegaEngine
{
	// forward declerations
	class World;
	class InputManager;

	class Engine
	{
	public:

		Engine(const char *winTitle, uint32_t width, uint32_t height);
		~Engine();

		void createWorld(std::string filename, std::string name);
		void createWindow(const char *winTitle);
		void loadConfigFile();

		void start_loop();

		// helper functions
		GLFWwindow* Window() const { return window; }

	private:

		// configuration for the omega engine
		EngineConfig engine_config;

		// glfw stuff
		GLFWwindow * window;
		const char *windowTitle;
		GLFWmonitor* monitor;
		const GLFWvidmode* vmode;
		
		// all keyboard, mouse, gamepad, ect. inputs dealt with here
		std::unique_ptr<InputManager> inputManager;

		// windw details set on init
		uint32_t windowWidth;
		uint32_t windowHeight;

		// a collection of worlds registered with the engine
		std::vector<std::unique_ptr<World> > worlds;
		uint32_t currentWorldIndex;		

		// a list of all grpahics devices that are available
		std::vector<VulkanAPI::Device> gfx_devices;
		uint8_t current_gfx_device;
	};

}

