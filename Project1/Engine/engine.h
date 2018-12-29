#pragma once
#include <vector>
#include <memory>

// forward declerations
class InputManager;
struct GLFWwindow;
struct GLFWmonitor;
struct GLFWvidmode;
class World;

namespace OmegaEngine
{
	class Engine
	{
	public:

		struct EngineConfig
		{
			bool useSSAO = false;
			bool useMSAA = false;
			bool shadowsEnabled = true;
			bool bloomEnabled = true;
			bool fogEnabled = true;
			bool showUi = false;

			enum class AntiAlaisingMode
			{

			};
			AntiAlaisingMode aaMode;

			float targetFrameRate = 30.0f;
		};

		Engine(const char *winTitle, uint32_t width, uint32_t height);
		~Engine();

		void createWorld(std::string filename, std::string name);
		void createWindow(const char *winTitle);
		void loadConfigFile(EngineConfig& config);

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
	};

}

