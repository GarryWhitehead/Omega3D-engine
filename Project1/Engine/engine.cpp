#include "Engine/engine.h"
#include "utility/logger.h"
#include "Utility/FileUtil.h"
#include "Engine/world.h"
#include "Engine/Omega_Global.h"
#include "Vulkan/Device.h"
#include "Vulkan/Common.h"

#include "glfw/glfw3.h"

#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"

namespace OmegaEngine
{
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
		loadConfigFile(engine_config);

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

		VK_CHECK_RESULT(volkInitialize());

		// now prepare the graphics device -  we can have multiple devices though this isn't fully implemented yet
		VulkanAPI::Device device;
		uint32_t instance_count;

		const char** instance_ext = glfwGetRequiredInstanceExtensions(&instance_count);
		device.createInstance(instance_ext, instance_count);
		device.prepareDevice();

		// create a which will be the abstract scrren surface which will be used for creating swapchains
		// TODO: add more cross-platform compatibility by adding more surfaces
		vk::SurfaceKHR surface;
		VK_CHECK_RESULT(glfwCreateWindowSurface(device.getInstance, window, nullptr, &surface.operator VkDeviceMemory));
		device.set_window_surface(surface);

		gfx_devices.push_back(device);
		current_gfx_device = gfx_devices.size() - 1;
	}

	void Engine::createWorld(std::string filename, std::string name)
	{
		std::unique_ptr<World> world = std::make_unique<World>(filename, name);

		worlds.push_back(world);
		currentWorldIndex = worlds.size() - 1;
	}

	void Engine::loadConfigFile(EngineConfig& config)
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

		if (doc.HasMember("General Gfx Settings")) {
			auto& settings = doc["General Gfx Settings"];
			if (doc.HasMember("Shadows Enabled")) {
				config.shadowsEnabled = doc["Shadows Enabled"].GetBool();
			}
			if (doc.HasMember("Bloom Enabled")) {
				config.bloomEnabled = doc["Bloom Enabled"].GetBool();
			}
			if (doc.HasMember("Fog Enabled")) {
				config.fogEnabled = doc["Fog Enabled"].GetBool();
			}
		}
	}
}





