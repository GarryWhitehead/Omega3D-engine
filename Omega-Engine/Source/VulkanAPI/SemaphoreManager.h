#pragma once
#include "VulkanAPI/Common.h"

namespace VulkanAPI
{
class VkContext;

class SemaphoreManager
{

public:

	SemaphoreManager(VkContext& context);
	~SemaphoreManager();

	vk::Semaphore getSemaphore();
	void recycle(vk::Semaphore semaphore);

private:

	VkContext& context;

	std::vector<vk::Semaphore> semaphores;
};

} // namespace VulkanAPI
