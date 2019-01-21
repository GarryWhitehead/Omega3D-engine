#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{

	class SemaphoreManager
	{

	public:

		SemaphoreManager(vk::Device dev);
		~SemaphoreManager();

	private:

		vk::Device device;
		std::vector<vk::Semaphore> semaphores;
	};

}

