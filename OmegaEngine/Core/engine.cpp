#include "Core/engine.h"
#include "Core/Omega_Global.h"
#include "Core/World.h"

#include "Utility/FileUtil.h"
#include "Utility/Timer.h"

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Device.h"

#include "utility/logger.h"

#include "external/rapidjson/document.h"
#include "external/rapidjson/stringbuffer.h"



using namespace std::chrono_literals;

namespace OmegaEngine
{

Engine::Engine()
{
	// load config file if there is one, otherwise use default settings
	loadConfigFile();

	uint32_t instanceCount;
	const char **instanceExt = glfwGetRequiredInstanceExtensions(&instanceCount);
	device->createInstance(instanceExt, instanceCount);

	// prepare the physical and abstract device including queues
	gfxDevice->prepareDevice();
}

Engine::~Engine()
{
}

World *Engine::createWorld(const std::string &name)
{
	// create an empty world
	std::unique_ptr<World> world = std::make_unique<World>();

	world->create(name);

	auto outputWorld = world.get();
	worlds.emplace(name, std::move(world));
	this->currentWorld = name;

	return outputWorld;
}

void Engine::loadConfigFile()
{
	std::string json;
	const char filename[] = "omega_engineConfig.ini"; // probably need to check the current dir here
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

		auto &world = worlds[currentWorld];
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

} // namespace OmegaEngine
