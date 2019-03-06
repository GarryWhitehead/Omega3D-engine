#include "Engine/engine.h"
#include "utility/logger.h"
#include "Utility/FileUtil.h"
#include "Utility/Timer.h"
#include "Engine/World.h"
#include "Engine/Omega_Global.h"
#include "Managers/InputManager.h"
#include "Vulkan/Device.h"
#include "Vulkan/Common.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"

#include "glfw/glfw3.h"

namespace OmegaEngine
{
	Engine::Engine()
	{

	}

	Engine::Engine(const char *win_title, uint32_t width, uint32_t height) :
		windowTitle(win_title),
		windowWidth(width),
		windowHeight(height)
	{
		// Create a new instance of glfw
		createWindow(win_title);

		// create all global instances including managers
		Global::init();
		
		// load config file if there is one, otherwise use default settings
		loadConfigFile();

		//create a new instance of the input manager
		inputManager = std::make_unique<InputManager>(window, width, height);
	}

	Engine::~Engine()
	{

	}

	void Engine::createWindow(const char *winTitle)
	{
		//glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit()) {
			LOGGER_ERROR("Critical error! Failed initialising GLFW. \n");
			throw std::runtime_error("Unable to initiliase GLFW");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		monitor = glfwGetPrimaryMonitor();
		vmode = glfwGetVideoMode(monitor);

		window = glfwCreateWindow(windowWidth, windowHeight, winTitle, nullptr, nullptr);
		if (!window) {
			LOGGER_ERROR("Critical error! Unable to open window!");
			throw std::runtime_error("Unable to open glfw window");
		}

		//VK_CHECK_RESULT(volkInitialize());

		// now prepare the graphics device -  we can have multiple devices though this isn't fully implemented yet
		VulkanAPI::Device device;
		uint32_t instance_count;

		const char** instance_ext = glfwGetRequiredInstanceExtensions(&instance_count);
		device.createInstance(instance_ext, instance_count);

		// create a which will be the abstract scrren surface which will be used for creating swapchains
		// TODO: add more cross-platform compatibility by adding more surfaces
		VkSurfaceKHR temp_surface;
		VkResult err = glfwCreateWindowSurface(device.getInstance(), window, nullptr, &temp_surface);
		if (err) {
			throw std::runtime_error("Unable to create window surface.");
		}
		vk::SurfaceKHR surface = vk::SurfaceKHR(temp_surface);
		device.set_window_surface(surface);

		// prepare the physical and abstract device including queues
		device.prepareDevice();

		gfx_devices.push_back(device);
		current_gfx_device = static_cast<uint8_t>(gfx_devices.size() - 1);
	}

	void Engine::createWorld(std::string filename, std::string name)
	{
		// create a world using a omega engine scene file
		std::unique_ptr<World> world = std::make_unique<World>(Managers::OE_MANAGERS_ALL, gfx_devices[current_gfx_device]);
		
		// throw an error here as calling a function for specifically creating a world with a scene file.
		if (!world->create(filename.c_str())) {
			LOGGER_ERROR("Unable to create world as no omega-scene file found.");
			throw std::runtime_error("Omega-scene file not found.");
		}

		worlds.push_back(std::move(world));
		currentWorldIndex = static_cast<uint32_t>(worlds.size() - 1);
	}

	void Engine::loadConfigFile()
	{
		std::string json;
		const char filename[] = "omega_engine_config.ini";		// probably need to check the current dir here
		if (!FileUtil::readFileIntoBuffer(filename, json)) {
			return;
		}

		// if we cant parse the confid, then go with the default values
		rapidjson::Document doc;
		if (doc.Parse(json.c_str()).HasParseError()) {
			return;			
		}

		// general engine settings
		if (doc.HasMember("FPS")) {
			engine_config.fps = doc["FPS"].GetFloat();
		}

	}

	void Engine::start_loop()
	{
		// convert delta time to ms
		const double dt = engine_config.fps / 1000;

		// fixed-step loop
		double accumulator = 0.0;
		double total_time = 0.0;

		Timer timer;
		timer.start_timer();

		while (Global::program_state.is_running())
		{
			auto elapsed_time = timer.get_time_elapsed(true);
			accumulator += static_cast<int>(elapsed_time.count());
			
			auto& world = worlds[currentWorldIndex];
			while (accumulator >= dt) {
				world->update(total_time, static_cast<double>(elapsed_time.count()));

				total_time += dt;
				accumulator -= dt;
			}

			double interpolation = accumulator / dt;
			world->render(interpolation);
		}
	}
}





