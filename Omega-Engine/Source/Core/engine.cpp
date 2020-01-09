#include "Core/Engine.h"

#include "Core/Omega_Global.h"
#include "Core/World.h"
#include "Core/Scene.h"

#include "utility/FileUtil.h"

#include "VulkanAPI/SwapChain.h"

#include "Types/NativeWindowWrapper.h"

#include "Rendering/Renderer.h"

#include "utility/Logger.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"

namespace OmegaEngine
{

OEEngine::OEEngine()
{
}

OEEngine::~OEEngine()
{
	for (OEWorld* world : worlds)
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

bool OEEngine::init(OEWindowInstance& window)
{
	surface = VulkanAPI::Swapchain::createSurface(window, vkDriver.getContext().getInstance());
    
    if(!vkDriver.init(window.extensions.first, window.extensions.second, surface.get()))
    {
        return false;
    }
    return true;
}

SwapchainHandle OEEngine::createSwapchain(OEWindowInstance& window)
{
	// create a swapchain for surface rendering based on the platform specific window surface
	VulkanAPI::Swapchain swapchain;
    swapchain.prepare(vkDriver.getContext(), surface);
    return window.addSwapchain(swapchain);
}

OEWorld* OEEngine::createWorld(Util::String name)
{
	// create an empty world
	OEWorld* world = new OEWorld(*this, vkDriver);

	world->prepare(name);

	worlds.emplace_back(std::move(world));
	this->currentWorld = name;

	return world;
}

Renderer* OEEngine::createRenderer(OEWindowInstance& window, SwapchainHandle& handle, OEScene* scene)
{
    VulkanAPI::Swapchain& swapchain = window.swapchains[handle.getHandle()];
    
    Renderer* renderer = new Renderer(*this, *scene, swapchain, config);
    assert(renderer);
    renderers.emplace_back(renderer);
    return renderer;
}

VulkanAPI::VkDriver& OEEngine::getVkDriver()
{
	return vkDriver;
}

// ** manager helper functions **
AnimationManager& OEEngine::getAnimManager()
{
	return *animManager;
}

OELightManager& OEEngine::getLightManager()
{
	return *lightManager;
}

OERenderableManager& OEEngine::getRendManager()
{
	return *rendManager;
}

TransformManager& OEEngine::getTransManager()
{
	return *transManager;
}

}    // namespace OmegaEngine
