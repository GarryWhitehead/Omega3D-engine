#pragma once

#include "VulkanAPI/Common.h"

#include "Types/NativeWindowWrapper.h"

using namespace OmegaEngine;

namespace VulkanAPI
{
// forward decleration
class Instance;

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
	SurfaceWrapper(NativeWindowWrapper& window, Instance& instance);
#endif

	uint32_t getWidth() const
	{
		return winWidth;
	}

	uint32_t getHeight() const
	{
		return winHeight;
	}


private:

	vk::SurfaceKHR surface;

	uint32_t winWidth = 0;
	uint32_t winHeight = 0;
};

}
}    // namespace VulkanAPI
