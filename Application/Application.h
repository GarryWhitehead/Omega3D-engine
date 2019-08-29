#pragma once

#include "VulkanAPI/Device.h"

#include <cstdint>

class Application
{
    
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /** 
     * initilaises the window and surface for rendering. Also prepares the vulkan backend.
     * @param width: window width in dpi; if zero will sets window width to fullscreen size 
     * @param height: window height in dpi; if zero will sets window height to fullscreen size
     * @return If everything is initialsied successfully, returns true
    */
    bool init(uint32_t width = 0, uint32_t height = 0);

private:

    /// the vulkan context used for surface rendering
    VulkanAPI::Device gfxDevice;

#ifdef PLATFORM_GLFW
    GlfwPlatform glfw;
#endif

};