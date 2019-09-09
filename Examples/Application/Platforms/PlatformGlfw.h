#pragma once

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "glfw/glfw3.h"

namespace Application
{
    class GlfwPlatform
    {
    public:

        GlfwPlatform();
        
        void createInstance();
        void createWindow();
        void createSurfaceKHR();

        void* GlfwPlatform::getNativeWinPointer();

    private:
        GLFWwindow *window = nullptr;
        GLFWmonitor *monitor = nullptr;
        const GLFWvidmode *vmode;
    }

}

