#include "Buffer.h"

namespace VulkanAPI
{
	namespace Util
	{
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags, vk::PhysicalDevice gpu)
		{
			vk::PhysicalDeviceMemoryProperties memoryProp = gpu.getMemoryProperties();

			for (uint32_t c = 0; c < memoryProp.memoryTypeCount; ++c)
			{
				if ((type & (1 << c)) && (memoryProp.memoryTypes[c].propertyFlags & flags) == flags)
					return c;
			}

			return UINT32_MAX;
		}
	}


	Buffer::Buffer()
	{
	}


	Buffer::~Buffer()
	{
	}

}
