#include "Application.h"
#include "utility/Logger.h"

void* Application::init(const char* title, uint32_t width, uint32_t height)
{
    // init glfw
    if (!glfw.init())
    {
        return nullptr;
    }

    // create a window
    glfw.setWindowDim(width, height);
    glfw.setWindowTitle(title);
    if (!glfw.createWindow())
    {
        return nullptr;
    }

    NativeWindowWrapper winWrapper;
    winWrapper.width = width;
    winWrapper.height = height;
    winWrapper.nativeWin = (void*)glfw.getNativeWinPointer();
    winWrapper.extensions = glfw.getInstanceExt();
}