#pragma once

#include "Platforms/PlatformGlfw.h"

#include "Types/NativeWindowWrapper.h"

#include <cstdint>
#include <tuple>

class Application
{
    
public:
    
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
	Application(Application&&) = delete;
	Application& operator=(Application&&) = delete;

    /** 
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param: Title to use for the window. Nullptr states no title bar
     * @param width: window width in dpi; if zero will sets window width to fullscreen size 
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns a native window pointer
    */
	bool init(const char* title, uint32_t width, uint32_t height,
	                       OmegaEngine::NativeWindowWrapper& output);

    void run();


private:

    GlfwPlatform glfw;

    // the running state of this app. Set to true by 'esc' keypress or window close
    bool closeApp = false;
};
