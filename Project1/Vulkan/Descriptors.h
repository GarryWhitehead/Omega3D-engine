#pragma once
#include "volk.h"

namespace Vulkan
{

	class Descriptors
	{

	public:

		Descriptors();
		~Descriptors();

		void initAllocInfo();
		void initSet();

	private:

		VkDescriptorSetAllocateInfo info;
		VkWriteDescriptorSet set;
		VkDescriptorSetLayout layout;
	};

}

