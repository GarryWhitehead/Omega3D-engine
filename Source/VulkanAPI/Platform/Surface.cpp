#include "Surface.h"

#include "VulkanAPI/VkDriver.h"

namespace VulkanAPI
{
namespace Platform
{

#ifdef VK_USE_PLATFORM_WIN32_KHR
SurfaceWrapper::SurfaceWrapper(NativeWindowWrapper& win, vk::Instance& instance)
{
	HWND windowPtr = static_cast<HWND>(win.nativeWin);
	vk::Win32SurfaceCreateInfoKHR createInfo{ {}, static_cast<HINSTANCE>(0), windowPtr };
	
	VK_CHECK_RESULT(instance.createWin32SurfaceKHR(&createInfo, nullptr, &surface));
}
#endif

}    // namespace Platform
}    // namespace VulkanAPI
