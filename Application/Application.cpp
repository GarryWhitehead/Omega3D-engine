#include "Application.h"
#include "utility/Logger.h"

bool Application::init(uint32_t width, uint32_t height)
{
#ifdef PLATFORM_GLFW

    // init glfw
    if (!glfw.init())
    {
        return false;
    }

    // create a window
    glfw.setWindowDim(width, height);
    glfw.setWindowTitle(winTitle);
    if (!glfw.createWindow())
    {
        return false;
    }

    // create a new instance of Vulkan
    glfw.createVkInstance(device);
    glfw.createSurafceKHR()

	// prepare the physical and abstract device including queues
	device->prepareDevice();

#else
    LOGGER_ERROR("Unsupported platform. Only the GLFW is supported at present.");
#endif

    return true;
}