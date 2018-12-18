#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

namespace VkInitilisation
{

	VkInstance CreateInstance();
	VkSurfaceKHR PrepareWindowSurface(VkInstance instance, GLFWwindow *window);

}

