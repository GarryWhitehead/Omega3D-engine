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

#ifndef ENGINE_H
#define ENGINE_H

#include "utility/Compiler.h"

#include "omega-engine/Scene.h"
#include "omega-engine/World.h"

#include "utility/CString.h"

namespace OmegaEngine
{
// forward declerations
class AnimationManager;
class LightManager;
class RenderableManager;
class TransformManager;
class Renderer;
class EngineConfig;

class SwapchainHandle
{
public:
    
    SwapchainHandle(uint32_t h) :
        handle(h)
    {}
    
    uint32_t getHandle() const
    {
        return handle;
    }
    
private:
    
    uint32_t handle;
};

/**
     * A wrapper containing all the information needed to create a swapchain.
     */
class WindowInstance
{
public:
    
    void* getNativeWindowPtr();
    
    uint32_t getWidth() const;
    
    uint32_t getHeight() const;
    
private:

};

class OE_PUBLIC Engine
{
public:

    /**
    * @brief Initialises a new vulkan context. This creates a new instance and
    * prepares the physical and abstract device and associated queues
    * Note: Only one vulkan device is allowed. Multiple devices supporting multi-gpu
    * setups is not yet supported
    */
    bool init(WindowInstance* window);

    void destroy();
    
    /**
    * @brief This creates a new swapchain instance based upon the platform-specific
    * ntaive window pointer created by the application
    */
    SwapchainHandle createSwapchain(WindowInstance* window);

    /**
    * @brief Creates a new renderer instance based on the user specified swapchain and scene
    */
    Renderer* createRenderer(SwapchainHandle& handle, Scene* scene);

    /**
    * @ brief Creates a new world object. This object is stored by the engine allowing
    * multiple worlds to created if desired.
    * @param name A string name used as a identifier for this world
    * @return Returns a pointer to the newly created world
    */
    World* createWorld(Util::String name);

protected:
    
    Engine() = default;
	~Engine() = default;

};

}

#endif /* ENGINE_HPP */
