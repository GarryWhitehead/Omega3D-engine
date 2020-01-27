#pragma once

#include "omega-engine/Application.h"

#include "Platforms/PlatformGlfw.h"

#include <cstdint>

// forward declerations
namespace OmegaEngine
{

class OEEngine;
class OEScene;
class OERenderer;
class OEWindowInstance;

class OEApplication : public Application
{
    
public:
    
    OEApplication() = default;

    OEApplication(const OEApplication&) = delete;
    OEApplication& operator=(const OEApplication&) = delete;

    static OEApplication* create(const char* title, uint32_t width, uint32_t height);

    static void destroy(OEApplication* app);

    OEEngine* createEngine(OEWindowInstance* window);

    /** 
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param: Title to use for the window. Nullptr states no title bar
     * @param width: window width in dpi; if zero will sets window width to fullscreen size 
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns a native window pointer
    */
	OEWindowInstance* init(const char* title, uint32_t width, uint32_t height);

    bool run(OEScene* scene, OERenderer* renderer);

    OEWindowInstance* getWindow();

private:

    // A engine instance. Only one permitted at the moment.
	OEEngine* engine = nullptr;

    // current window - maybe we should allow multiple window instances?
	OEWindowInstance* winInstance = nullptr;

    GlfwPlatform glfw;

    // the running state of this app. Set to true by 'esc' keypress or window close
    bool closeApp = false;
};

}
