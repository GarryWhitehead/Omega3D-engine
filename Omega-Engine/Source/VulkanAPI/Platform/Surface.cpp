#include "Surface.h"

#include "Types/NativeWindowWrapper.h"

#include "VulkanAPI/VkDriver.h"

namespace VulkanAPI
{
namespace Platform
{

#ifdef VK_USE_PLATFORM_WIN32_KHR
SurfaceWrapper::SurfaceWrapper(OmegaEngine::OEWindowInstance& win, vk::Instance& instance)
{
	vk::Win32SurfaceCreateInfoKHR createInfo{ {}, (HINSTANCE)0, (HWND)win.getNativeWindowPtr() };
	
	VK_CHECK_RESULT(instance.createWin32SurfaceKHR(&createInfo, nullptr, &surface));
}
#endif

}    // namespace Platform
}    // namespace VulkanAPI
