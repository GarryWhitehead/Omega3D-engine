#include "Surface.h"

#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{
namespace Platform
{

#ifdef VK_USE_PLATFORM_WIN32_KHR
SurfaceWrapper::SurfaceWrapper(NativeWindowWrapper& win, Instance& instance)
{
	HWND window = static_cast<HWND>(win.nativeWin);
	vk::Win32SurfaceCreateInfoKHR createInfo{ 0, window, GetModuleHandle(nullptr) };
	
	VK_CHECK_RESULT(instance.createWin32SurfaceKHR(&createInfo, nullptr, &surface));
}
#endif

}    // namespace Platform
}    // namespace VulkanAPI