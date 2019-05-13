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

using namespace std::chrono_literals;

namespace OmegaEngine
{
	Engine::Engine(std::string win_title, uint32_t width, uint32_t height) :
		windowTitle(win_title),
		windowWidth(width),
		windowHeight(height)
	{
		// Create a new instance of glfw
		createWindow(windowTitle);

		// create all global instances including managers
		Global::init();
		
		// load config file if there is one, otherwise use default settings
		loadConfigFile();

		//create a new instance of the input manager
		inputManager = std::make_unique<InputManager>(window, width, height);

		program_state = std::make_unique<ProgramState>();
	}

	Engine::~Engine()
	{

	}

	void Engine::createWindow(std::string winTitle)
	{
		//glfwSetErrorCallback(glfw_error_callback);
		if (!glfwInit()) 
		{
			LOGGER_ERROR("Critical error! Failed initialising GLFW. \n");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		monitor = glfwGetPrimaryMonitor();
		vmode = glfwGetVideoMode(monitor);

		window = glfwCreateWindow(windowWidth, windowHeight, winTitle.c_str(), nullptr, nullptr);
		if (!window) 
		{
			LOGGER_ERROR("Critical error! Unable to open window!");
		}

		// now prepare the graphics device -  we can have multiple devices though this isn't fully implemented yet
		auto& device = std::make_unique<VulkanAPI::Device>();
		uint32_t instance_count;

		const char** instance_ext = glfwGetRequiredInstanceExtensions(&instance_count);
		device->createInstance(instance_ext, instance_count);

		// create a which will be the abstract scrren surface which will be used for creating swapchains
		// TODO: add more cross-platform compatibility by adding more surfaces
		VkSurfaceKHR temp_surface;
		VkResult err = glfwCreateWindowSurface(device->getInstance(), window, nullptr, &temp_surface);
		if (err) 
		{
			LOGGER_ERROR("Unable to create window surface.");
		}
		vk::SurfaceKHR surface = vk::SurfaceKHR(temp_surface);
		device->set_window_surface(surface);

		// prepare the physical and abstract device including queues
		device->prepareDevice();

		gfx_devices.emplace_back(std::move(device));
		current_gfx_device = static_cast<uint32_t>(gfx_devices.size() - 1);
	}

	void Engine::createWorld(std::string filename, std::string name)
	{
		// create a world using a omega engine scene file
		std::unique_ptr<World> world = std::make_unique<World>(Managers::OE_MANAGERS_ALL, gfx_devices[current_gfx_device], engine_config);
		
		// throw an error here as calling a function for specifically creating a world with a scene file.
		if (!world->create(filename.c_str())) 
		{
			LOGGER_ERROR("Unable to create world as no omega-scene file found.");
		}

		worlds.push_back(std::move(world));
		currentWorldIndex = static_cast<uint32_t>(worlds.size() - 1);
	}

	void Engine::loadConfigFile()
	{
		std::string json;
		const char filename[] = "omega_engine_config.ini";		// probably need to check the current dir here
		if (!FileUtil::readFileIntoBuffer(filename, json)) 
		{
			return;
		}

		// if we cant parse the confid, then go with the default values
		rapidjson::Document doc;
		if (doc.Parse(json.c_str()).HasParseError()) 
		{
			return;			
		}

		// general engine settings
		if (doc.HasMember("FPS")) 
		{
			engine_config.fps = doc["FPS"].GetFloat();
		}
		if (doc.HasMember("Screen Width")) 
		{
			engine_config.screen_width = doc["Screen Width"].GetInt();
		}
		if (doc.HasMember("Screen Height")) 
		{
			engine_config.screen_height = doc["Screen Height"].GetInt();
		}
		if (doc.HasMember("Mouse Sensitivity")) 
		{
			engine_config.mouse_sensitivity = doc["Mouse Sensitivity"].GetFloat();
		}
	}

	void Engine::start_loop()
	{
		program_state->set_running();
		
		// convert delta time to ms
		const std::chrono::nanoseconds time_step(16ms);

		// fixed-step loop
		std::chrono::nanoseconds accumulator(0ns);
		double total_time = 0.0;

		Timer timer;
		timer.start_timer();

		while (program_state->is_running())
		{
			auto elapsed_time = timer.get_time_elapsed(true);
			accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time);

			auto& world = worlds[currentWorldIndex];
			while (accumulator >= time_step) 
			{
				// poll for any input
				inputManager->update();

				// update everything else
				world->update(total_time, static_cast<double>(elapsed_time.count()));

				total_time += static_cast<double>(time_step.count());
				accumulator -= time_step;
				//printf("updated!\n");
			}

			double interpolation = (double)accumulator.count() / (double)time_step.count();
			world->render(interpolation);
			//printf("rendered!\n");
		}
	}
}





