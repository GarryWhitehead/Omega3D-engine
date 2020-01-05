#pragma once

#include "VulkanAPI/Common.h"

#include "Types/NativeWindowWrapper.h"

using namespace OmegaEngine;

namespace VulkanAPI
{

namespace Platform
{

/**
* @brief A wrapper for platform specific vulkan surface objects
*/
class SurfaceWrapper
{
public:
#ifdef VK_USE_PLATFORM_WIN32_KHR
	using HWND = void*;
#endif
    
    SurfaceWrapper() = default;
    
    SurfaceWrapper(NativeWindowWrapper& window, vk::Instance& instance);
    
	uint32_t getWidth() const
	{
		return winWidth;
	}

	uint32_t getHeight() const
	{
		return winHeight;
	}

	vk::SurfaceKHR& get()
	{
		return surface;
	}

private:
	vk::SurfaceKHR surface;

	uint32_t winWidth = 0;
	uint32_t winHeight = 0;
};

}    // namespace Platform
}    // namespace VulkanAPI
