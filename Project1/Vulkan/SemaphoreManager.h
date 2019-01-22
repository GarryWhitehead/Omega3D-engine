#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{

	class SemaphoreManager
	{

	public:

		SemaphoreManager();
		SemaphoreManager(vk::Device dev);
		~SemaphoreManager();

		vk::Semaphore get_semaphore();
		void recycle(vk::Semaphore semaphore);
		
		void init(vk::Device dev)
		{
			device = dev;
		}

	private:

		vk::Device device;
		std::vector<vk::Semaphore> semaphores;
	};

}

