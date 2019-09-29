#include "Core/engine.h"
#include "Core/Omega_Global.h"
#include "Core/World.h"

#include "Utility/FileUtil.h"

#include "VulkanAPI/Platform/Surface.h"

#include "utility/logger.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

namespace OmegaEngine
{

Engine::Engine()
{
	// load config file if there is one, otherwise use default settings
	loadConfigFile();
}

Engine::~Engine()
{
	for (World* world : worlds)
	{
		if (world)
		{
			delete world;
		}
	}
}

bool Engine::initDevice(NativeWindowWrapper& window)
{
	// create a new vulkan instance
	const char** instanceExt;
	uint32_t count;
	std::tie(instanceExt, count) = window.extensions;
	context.createInstance(instanceExt, count);
	
	// prepare the physical and abstract device including queues
	context.prepareDevice();

	// create a swapchain for surface rendering based on the platform specific window surface
	VulkanAPI::Platform::SurfaceWrapper surface = VulkanAPI::Swapchain::createSurface(window, instance);
	swapchain.prepare(context, surface);
}

World* Engine::createWorld(Util::String name)
{
	// create an empty world
	World* world = new World;

	world->prepare(name);

	worlds.emplace_back(std::move(world));
	this->currentWorld = name;

	// not exactly unique - maybe use a raw pointer here
	return world;
}

void Engine::loadConfigFile()
{
	std::string json;
	const char filename[] = "omega_engineConfig.ini";    // probably need to check the current dir here
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
	const std::chrono::nanoseconds timeStep(33ms);

	// fixed-step loop
	std::chrono::nanoseconds accumulator(0ns);
	double totalTime = 0.0;

	Timer timer;
	timer.startTimer();

	while (programState.getIsRunning())
	{
		auto elapsedTime = timer.getTimeElapsed(true);
		accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(elapsedTime);

		// poll for any input
		inputManager->update();

		auto& world = worlds[currentWorld];
		while (accumulator >= timeStep)
		{
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

}    // namespace OmegaEngine
