#pragma once
#include "VulkanAPI/Common.h"

namespace VulkanAPI
{

class SemaphoreManager
{

public:

	SemaphoreManager(vk::Device dev);
	~SemaphoreManager();

	vk::Semaphore getSemaphore();
	void recycle(vk::Semaphore semaphore);

private:

	vk::Device device;
	std::vector<vk::Semaphore> semaphores;
};

} // namespace VulkanAPI
