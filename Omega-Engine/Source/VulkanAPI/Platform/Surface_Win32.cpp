#include "Surface.h"

#ifdef VK_USE_PLATFORM_WIN32_KHR

namespace VulkanAPI
{
namespace Platform
{

SurfaceWrapper::SurfaceWrapper(OmegaEngine::OEWindowInstance& win, vk::Instance& instance)
{
    vk::Win32SurfaceCreateInfoKHR createInfo{ {}, (HINSTANCE)0, (HWND)win.getNativeWindowPtr() };
    
    VK_CHECK_RESULT(instance.createWin32SurfaceKHR(&createInfo, nullptr, &surface));
}

}    // namespace Platform
}    // namespace VulkanAPI

#endif
