#include "SemaphoreManager.h"

namespace VulkanAPI
{

	SemaphoreManager::SemaphoreManager(vk::Device dev) :
		device(dev)
	{
	}


	SemaphoreManager::~SemaphoreManager()
	{
	}

	vk::Semaphore SemaphoreManager::get_semaphore()
	{
		vk::Semaphore semaphore;

		if (!semaphores.empty()) {
			semaphore = semaphores.back();
			semaphores.pop_back();
		}
		else {
			// create a new semaphore
			vk::SemaphoreCreateInfo create_info;
			VK_CHECK_RESULT(device.createSemaphore(&create_info, nullptr, &semaphore));
		}
		return semaphore;
	}

	void SemaphoreManager::recycle(vk::Semaphore semaphore)
	{
		semaphores.push_back(semaphore);
	}
}
