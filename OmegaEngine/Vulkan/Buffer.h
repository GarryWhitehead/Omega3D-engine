#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{
	namespace Util
	{
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags, vk::PhysicalDevice gpu);
	}

	// forward decleartions

	enum class BufferType
	{
		Uniform,
		Storage,
		Count
	};

	class Buffer
	{

	public:

		Buffer();
		~Buffer();

	private:

		uint32_t offset;
		uint32_t segment_id;		// indiactes which segemnt this buffer is stored in 
		BufferType type;
		bool isInit = false;
	};

}

