#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	namespace Util
	{
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags, vk::PhysicalDevice gpu);
	}

	class Buffer
	{

	public:

		Buffer();
		~Buffer();

	private:

	};

}

