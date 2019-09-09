#pragma once

#include "VulkanAPI/Device.h"

#include "Platforms/PlatformGlfw.h"

#include <cstdint>
#include <tuple>

class Application
{
    
public:
    
    /**
     * A wrapper containing all the information needed to create a swapchain.
     */
    struct NativeWindowWrapper
    {
        void *nativeWin;
        uint32_t width;
        uint32_t height;
        std::tuple<const char**, uint32_t> extensions;
    };

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
	bool Application::init(const char* title, uint32_t width, uint32_t height,
	                       Application::NativeWindowWrapper& output);

private:

    GlfwPlatform glfw;


};