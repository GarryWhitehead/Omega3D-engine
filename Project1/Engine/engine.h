#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <assert.h>

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

		static constexpr float DT = 1.0f / 30.0f;

		Engine(const char *winTitle, uint32_t width, uint32_t height);
		~Engine();

		// main core functions
		void init();
		void update(int acc_time);
		void release();
		void render(float interpolation);
		void createWorld(std::string filename, std::string name);
		void createWindow(const char *winTitle);

		// helper functions
		GLFWwindow* Window() const { return window; }

	private:

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

		bool isRunning;

		// a collection of worlds registered with the engine
		std::vector<std::unique_ptr<World> > worlds;
		uint32_t currentWorldIndex;							// current world which will be rendered, updated, etc.
	};

}

