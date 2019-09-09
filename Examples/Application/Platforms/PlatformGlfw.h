#pragma once

namespace Application
{
    class GlfwPlatform
    {
        GlfwPlatform() {}
        
        void createInstance();
        void createWindow();
        void createSurfaceKHR();
    }

}

