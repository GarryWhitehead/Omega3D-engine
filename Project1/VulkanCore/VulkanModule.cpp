#include "VulkanModule.h"


VulkanModule::VulkanModule(VulkanUtility *utility, VkMemoryManager *memory) :
	vkUtility(utility),
	p_vkMemory(memory)
{
}


VulkanModule::~VulkanModule()
{
}
