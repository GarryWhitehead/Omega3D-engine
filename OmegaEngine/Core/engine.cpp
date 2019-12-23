#include "Core/engine.h"

#include "Core/Omega_Global.h"
#include "Core/World.h"
#include "Core/Scene.h"

#include "Utility/FileUtil.h"

#include "VulkanAPI/Platform/Surface.h"
#include "VulkanAPI/SwapChain.h"

#include "Rendering/Renderer.h"

#include "utility/Logger.h"

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

bool Engine::init(NativeWindowWrapper& window)
{
	if(!vkDriver.init(window.extensions.first, window.extensions.second))
    {
        return false;
    }
    return true;
}

VulkanAPI::Swapchain Engine::createSwapchain(NativeWindowWrapper& window)
{
	// create a swapchain for surface rendering based on the platform specific window surface
	VulkanAPI::Swapchain swapchain;

	VulkanAPI::Platform::SurfaceWrapper surface = VulkanAPI::Swapchain::createSurface(window, vkDriver.getContext().getInstance());
	swapchain.prepare(vkDriver.getContext(), surface);

	return swapchain;
}

World* Engine::createWorld(Util::String name)
{
	// create an empty world
	World* world = new World(*this, vkDriver);

	world->prepare(name);

	worlds.emplace_back(std::move(world));
	this->currentWorld = name;

	return world;
}

Renderer* Engine::createRenderer(VulkanAPI::Swapchain& swapchain, Scene* scene)
{
    Renderer* renderer = new Renderer(*this, *scene, swapchain);
    
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

VulkanAPI::VkDriver& Engine::getVkDriver()
{
	return vkDriver;
}

// ** manager helper functions **
AnimationManager& Engine::getAnimManager()
{
	return *animManager;
}

CameraManager& Engine::getCameraManager()
{
	return *cameraManager;
}

LightManager& Engine::getLightManager()
{
	return *lightManager;
}

RenderableManager& Engine::getRendManager()
{
	return *rendManager;
}

TransformManager& Engine::getTransManager()
{
	return *transManager;
}

}    // namespace OmegaEngine
