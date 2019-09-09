#include "Application.h"

 bool Application::init(const char* title, uint32_t width, uint32_t height, Application::NativeWindowWrapper& output)
{
    // init glfw
    if (!glfw.init())
    {
        return nullptr;
    }

    // create a window
    if (!glfw.createWindow(width, height, title))
    {
        return nullptr;
    }

    output.width = width;
    output.height = height;
    output.nativeWin = (void*)glfw.getNativeWinPointer();
    output.extensions = glfw.getInstanceExt();
}