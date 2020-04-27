/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "Core/engine.h"

#include "Core/Scene.h"
#include "Core/World.h"
#include "Rendering/Renderer.h"
#include "Types/NativeWindowWrapper.h"
#include "VulkanAPI/SwapChain.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "utility/FileUtil.h"
#include "utility/Logger.h"

namespace OmegaEngine
{
OEEngine::OEEngine()
    : vkDriver(std::make_unique<VulkanAPI::VkDriver>())
    , transManager(std::make_unique<TransformManager>())
    , rendManager(std::make_unique<OERenderableManager>(*this))
    , animManager(std::make_unique<AnimationManager>())
    , lightManager(std::make_unique<OELightManager>())
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

bool OEEngine::init(OEWindowInstance* window)
{
    if (!vkDriver->createInstance(window->extensions.first, window->extensions.second))
    {
        LOGGER_ERROR("Fatal Error whilst creating Vulkan instance.");
        return false;
    }

    surface = VulkanAPI::Swapchain::createSurface(window, vkDriver->getContext().instance);

    if (!vkDriver->init(surface.get()))
    {
        LOGGER_ERROR("Fatal Error whilst preparing vulkan device.");
        return false;
    }
    return true;
}

void OEEngine::destroy()
{
    for (auto& swapchain : swapchains)
    {
        swapchain->destroy(vkDriver->getContext());
    }
}

SwapchainHandle OEEngine::createSwapchain(OEWindowInstance* window)
{
    // create a swapchain for surface rendering based on the platform specific window surface
    auto sc = std::make_unique<VulkanAPI::Swapchain>();
    sc->prepare(vkDriver->getContext(), surface);
    swapchains.emplace_back(std::move(sc));
    return SwapchainHandle {static_cast<uint32_t>(swapchains.size() - 1)};
}

OEWorld* OEEngine::createWorld(Util::String name)
{
    // create an empty world
    OEWorld* world = new OEWorld(*this, *vkDriver);

    world->prepare(name);

    worlds.emplace_back(std::move(world));
    this->currentWorld = name;

    return world;
}

Renderer* OEEngine::createRenderer(SwapchainHandle& handle, OEScene* scene)
{
    auto& swapchain = swapchains[handle.getHandle()];

    OERenderer* renderer = new OERenderer(*this, *scene, *swapchain, config);
    assert(renderer);
    renderers.emplace_back(renderer);
    return renderer;
}

VulkanAPI::VkDriver& OEEngine::getVkDriver()
{
    return *vkDriver;
}

// ** manager helper functions **
AnimationManager& OEEngine::getAnimManager()
{
    return *animManager;
}

OELightManager* OEEngine::getLightManager()
{
    return lightManager.get();
}

OERenderableManager* OEEngine::getRendManager()
{
    return rendManager.get();
}

TransformManager& OEEngine::getTransManager()
{
    return *transManager;
}

// ========================== front-end =========================================

bool Engine::init(WindowInstance* window)
{
    return static_cast<OEEngine*>(this)->init(static_cast<OEWindowInstance*>(window));
}

SwapchainHandle Engine::createSwapchain(WindowInstance* window)
{
    return static_cast<OEEngine*>(this)->createSwapchain(static_cast<OEWindowInstance*>(window));
}

Renderer* Engine::createRenderer(SwapchainHandle& handle, Scene* scene)
{
    return static_cast<OEEngine*>(this)->createRenderer(handle, static_cast<OEScene*>(scene));
}

World* Engine::createWorld(Util::String name)
{
    return static_cast<OEEngine*>(this)->createWorld(name);
}

void Engine::destroy()
{
    static_cast<OEEngine*>(this)->destroy();
}

} // namespace OmegaEngine
