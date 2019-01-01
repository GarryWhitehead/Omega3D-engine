#pragma once
#include "VulkanCore/vulkan_utility.h"

class MemoryAllocator;

class VulkanModule
{

public:

	VulkanModule(MemoryAllocator *memory);
	~VulkanModule();

	virtual void Init() = 0;
	virtual void Update(int acc_time) = 0;
	virtual void Destroy() = 0;

protected:

	// all modules have access to the vulkan memory management class
	MemoryAllocator *p_vkMemory;
};

