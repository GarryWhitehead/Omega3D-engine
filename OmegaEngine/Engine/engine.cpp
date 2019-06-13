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
	Engine::Engine(const char* title, uint32_t width, uint32_t height) :
		windowWidth(width),
		windowHeight(height)
	{
		std::strcpy(windowTitle, title);

		// Create a new instance of glfw
		createWindow(windowTitle);

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

	void Engine::createWindow(const char* winTitle)
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

		window = glfwCreateWindow(windowWidth, windowHeight, winTitle, nullptr, nullptr);
		if (!window) 
		{
			LOGGER_ERROR("Critical error! Unable to open window!");
		}

		// now prepare the graphics device -  we can have multiple devices though this isn't fully implemented yet
		auto& device = std::make_unique<VulkanAPI::Device>();
		uint32_t instanceCount;

		const char** instanceExt = glfwGetRequiredInstanceExtensions(&instanceCount);
		device->createInstance(instanceExt, instanceCount);

		// create a which will be the abstract scrren surface which will be used for creating swapchains
		// TODO: add more cross-platform compatibility by adding more surfaceCount
		VkSurfaceKHR tempSurface;
		VkResult err = glfwCreateWindowSurface(device->getInstance(), window, nullptr, &tempSurface);
		if (err) 
		{
			LOGGER_ERROR("Unable to create window surface.");
		}
		device->setWindowSurface(vk::SurfaceKHR(tempSurface));

		// prepare the physical and abstract device including queues
		device->prepareDevice();

		vkDevices.emplace_back(std::move(device));
		currentVkDevice = static_cast<uint32_t>(vkDevices.size() - 1);
	}

	World* Engine::createWorld(const char* filename, const char* name)
	{
		// create a world using a omega engine scene file
		std::unique_ptr<World> world = std::make_unique<World>(Managers::OE_MANAGERS_ALL, vkDevices[currentVkDevice], engineConfig);
		
		// throw an error here as calling a function for specifically creating a world with a scene file.
		if (!world->create(filename, name)) 
		{
			LOGGER_ERROR("Unable to create world as no omega-scene file found.");
		}

		auto outputWorld = world.get();
		worlds.emplace(name, std::move(world));
		this->currentWorld = name;

		return outputWorld;
	}

	World* Engine::createWorld(const char* name)
	{
		// create an empty world
		std::unique_ptr<World> world = std::make_unique<World>(Managers::OE_MANAGERS_ALL, vkDevices[currentVkDevice], engineConfig);

		world->create(name);
		
		auto outputWorld = world.get();
		worlds.emplace(name, std::move(world));
		this->currentWorld = name;

		return outputWorld;
	}

	void Engine::loadConfigFile()
	{
		std::string json;
		const char filename[] = "omega_engineConfig.ini";		// probably need to check the current dir here
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
			engineConfig.fps = doc["FPS"].GetFloat();
		}
		if (doc.HasMember("Screen Width")) 
		{
			engineConfig.screenWidth = doc["Screen Width"].GetInt();
		}
		if (doc.HasMember("Screen Height")) 
		{
			engineConfig.screenHeight = doc["Screen Height"].GetInt();
		}
		if (doc.HasMember("Mouse Sensitivity")) 
		{
			engineConfig.mouseSensitivity = doc["Mouse Sensitivity"].GetFloat();
		}
	}

	void Engine::startLoop()
	{
		programState.setRunning();
		
		// convert delta time to ms
		const std::chrono::nanoseconds timeStep(16ms);

		// fixed-step loop
		std::chrono::nanoseconds accumulator(0ns);
		double totalTime = 0.0;

		Timer timer;
		timer.startTimer();

		while (programState.getIsRunning())
		{
			auto elapsedTime = timer.getTimeElapsed(true);
			accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(elapsedTime);

			auto& world = worlds[currentWorld];
			while (accumulator >= timeStep) 
			{
				// poll for any input
				inputManager->update();

				// update everything else
				world->update(totalTime, static_cast<double>(elapsedTime.count()));

				totalTime += static_cast<double>(timeStep.count());
				accumulator -= timeStep;
				//printf("updated!\n");
			}

			double interpolation = (double)accumulator.count() / (double)timeStep.count();
			world->render(interpolation);
			//printf("rendered!\n");
		}
	}
}





