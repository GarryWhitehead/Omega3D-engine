#include "Core/Engine.h"

#include "Core/Omega_Global.h"
#include "Core/World.h"
#include "Core/Scene.h"

#include "utility/FileUtil.h"

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
    worlds.clear();
    
    for (Renderer* rend : renderers)
    {
        if (rend)
        {
            delete rend;
        }
    }
    renderers.clear();
}

bool Engine::init(NativeWindowWrapper& window)
{
	surface = VulkanAPI::Swapchain::createSurface(window, vkDriver.getContext().getInstance());
    
    if(!vkDriver.init(window.extensions.first, window.extensions.second, surface.get()))
    {
        return false;
    }
    return true;
}

VulkanAPI::Swapchain Engine::createSwapchain()
{
	// create a swapchain for surface rendering based on the platform specific window surface
	VulkanAPI::Swapchain swapchain;
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
    Renderer* renderer = new Renderer(*this, *scene, swapchain, config);
    assert(renderer);
    renderers.emplace_back(renderer);
    return renderer;
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
