#pragma once

#include "VulkanAPI/Common.h"

#include "Types/NativeWindowWrapper.h"

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
	
	SurfaceWrapper(NativeWindowWrapper& window);

private:

	vk::SurfaceKHR surface;
};

}
}    // namespace VulkanAPI
