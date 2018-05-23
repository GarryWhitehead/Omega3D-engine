#pragma once
#include "VulkanCore/vulkan_utility.h"

class VulkanModule
{

public:

	VulkanModule(VulkanUtility *utility);
	~VulkanModule();

	virtual void Update(int acc_time) = 0;
	virtual void Destroy() = 0;

protected:

	VulkanUtility *vkUtility;
};

