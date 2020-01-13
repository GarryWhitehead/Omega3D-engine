#pragma once

#include "VulkanAPI/Common.h"

#include "Types/NativeWindowWrapper.h"

namespace VulkanAPI
{

namespace Platform
{

/**
* @brief A base wrapper for platform specific vulkan surface objects
*/
class SurfaceWrapper
{
public:

    SurfaceWrapper() = default;
    SurfaceWrapper(OmegaEngine::OEWindowInstance& window, vk::Instance& instance);
    
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

public:
    
	vk::SurfaceKHR surface;

	uint32_t winWidth = 0;
	uint32_t winHeight = 0;
};

}    // namespace Platform
}    // namespace VulkanAPI
