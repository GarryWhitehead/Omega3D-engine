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

	vk::Semaphore SemaphoreManager::getSemaphore()
	{
		vk::Semaphore semaphore;

		if (!semaphores.empty()) {
			semaphore = semaphores.back();
			semaphores.pop_back();
		}
		else {
			// create a new semaphore
			vk::SemaphoreCreateInfo createInfo;
			VK_CHECK_RESULT(device.createSemaphore(&createInfo, nullptr, &semaphore));
		}
		return semaphore;
	}

	void SemaphoreManager::recycle(vk::Semaphore semaphore)
	{
		semaphores.push_back(semaphore);
	}
}
