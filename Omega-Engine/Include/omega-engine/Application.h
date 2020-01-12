#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "omega-engine/Engine.h"
#include "omega-engine/Scene.h"
#include "omega-engine/Renderer.h"

#include "utility/Compiler.h"

#include <cstdint>

namespace OmegaEngine
{

class Application
{
    
public:

    static Application* create(const char* title, uint32_t width, uint32_t height);

    static void destroy(Application* app);

    Engine* createEngine(WindowInstance* window);

    /**
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param: Title to use for the window. Nullptr states no title bar
     * @param width: window width in dpi; if zero will sets window width to fullscreen size
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns a native window pointer
    */
	WindowInstance* init(const char* title, uint32_t width, uint32_t height);

    void run(Scene* scene, Renderer* renderer);

    WindowInstance* getWindow();

protected:

    Application() = default;

};

}

#endif /* APPLICATION_HPP */
