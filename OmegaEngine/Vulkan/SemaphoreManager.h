#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{

	class SemaphoreManager
	{

	public:

		SemaphoreManager(vk::Device dev);
		~SemaphoreManager();

		vk::Semaphore getSemaphore();
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

