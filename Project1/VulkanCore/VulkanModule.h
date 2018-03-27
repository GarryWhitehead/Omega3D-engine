#pragma once
#include "VulkanCore/vulkan_utility.h"

class VulkanModule
{

public:

	VulkanModule();
	~VulkanModule();

	virtual void Destroy() = 0;

protected:

	VulkanUtility vkUtility;
};

