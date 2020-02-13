#include "Application.h"

#include "Core/Engine.h"
#include "Core/Scene.h"
#include "Rendering/Renderer.h"
#include "Types/NativeWindowWrapper.h"
#include "omega-engine/Engine.h"
#include "utility/Logger.h"
#include "utility/Timer.h"

#include <thread>

using namespace std::literals::chrono_literals;

namespace OmegaEngine
{

OEApplication* OEApplication::create(const char* title, uint32_t width, uint32_t height)
{
    // create a new instance
    OEApplication* app = new OEApplication();

    if (!app->init(title, width, height))
    {
        LOGGER_ERROR("Fatal Error. Unbale to create an instance pf OE application.");
        return nullptr;
    }

    return app;
}

OEEngine* OEApplication::createEngine(OEWindowInstance* window)
{
    OEEngine* eng = new OEEngine();
    if (!eng)
    {
        LOGGER_ERROR("Unable to crerate an engine instance");
        return nullptr;
    }
    if (!eng->init(window))
    {
        LOGGER_ERROR("Fatal Error. Unable to initialiase the engine.");
        return nullptr;
    }
    return eng;
}

OEWindowInstance* OEApplication::init(const char* title, uint32_t width, uint32_t height)
{
    // init glfw
    if (!glfw.init())
    {

        LOGGER_ERROR("Unable to initilaise glfw.");
        return nullptr;
    }

    // create a window
    if (!glfw.createWindow(width, height, title))
    {
        LOGGER_ERROR("Unable to create a glfw window.");
        return nullptr;
    }

    winInstance = new OEWindowInstance();
    assert(winInstance);

    winInstance->width = width;
    winInstance->height = height;
    winInstance->nativeWin = (void*) glfw.getNativeWinPointer();
    winInstance->extensions = glfw.getInstanceExt();

    return winInstance;
}

bool OEApplication::run(OEScene* scene, OERenderer* renderer)
{
    // convert delta time to ms
    const NanoSeconds frameTime(33ms);

    Util::Timer<NanoSeconds> timer;

    NanoSeconds appStartTime = timer.getCurrentTime();

    while (!closeApp)
    {
        NanoSeconds frameTime = timer.getCurrentTime();

        // check for any input from the window
        glfw.poll();

        // update the scene
        if (!scene->update(frameTime.count()))
        {
            return false;
        }

        // and the renderer
        if (!renderer->update())
        {
            return false;
        }

        // user define pre-render callback to be added here (or virtual)

        // TODP: multi view option- with each view drawn.
        // begin the rendering for this frame
        renderer->draw();

        // user defined post-render callback to be added here

        // calculate whether we have any time remaining this frame
        NanoSeconds endTime = timer.getCurrentTime();
        NanoSeconds elapsedTime = endTime - frameTime;

        // if we haven't used up the frame time, sleep for remainder
        if (elapsedTime < frameTime)
        {
            std::this_thread::sleep_for(frameTime - elapsedTime);
        }
    }
}

void OEApplication::destroy(OEApplication* app)
{
    if (app)
    {
        delete app;
        app = nullptr;
    }
}

OEWindowInstance* OEApplication::getWindow()
{
    assert(winInstance);
    return winInstance;
}

// ===================== front-end =========================================

WindowInstance* Application::init(const char* title, uint32_t width, uint32_t height)
{
    return static_cast<OEApplication*>(this)->init(title, width, height);
}

bool Application::run(Scene* scene, Renderer* renderer)
{
    return static_cast<OEApplication*>(this)->run(
        static_cast<OEScene*>(scene), static_cast<OERenderer*>(renderer));
}

Application* Application::create(const char* title, uint32_t width, uint32_t height)
{
    return OEApplication::create(title, width, height);
}

Engine* Application::createEngine(WindowInstance* window)
{
    return static_cast<OEApplication*>(this)->createEngine(static_cast<OEWindowInstance*>(window));
}

void Application::destroy(Application* app)
{
    OEApplication::destroy(static_cast<OEApplication*>(app));
}

WindowInstance* Application::getWindow()
{
    return static_cast<OEApplication*>(this)->getWindow();
}

} // namespace OmegaEngine
