#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

// forward declerations
class ValidationLayers;

class VulkanInstance
{

public:

	VulkanInstance();
	~VulkanInstance();

	void CreateInstance();
	void PrepareWindowSurface(GLFWwindow *window);

	VkInstance instance;
	VkSurfaceKHR surface;

private:

};

