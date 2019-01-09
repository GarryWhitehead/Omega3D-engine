#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/MemoryAllocator.h"

#include <memory>
#include <unordered_map>

namespace VulkanAPI
{
	// forward declerations
	class MemoryAllocator;
	class Buffer;
	enum class BufferType;

	enum class BufferMemoryType
	{
		Host,
		Local,
		Count
	};


	class BufferManager
	{

	public:

		struct BufferSegmentInfo
		{
			MemoryAllocator::SegmentInfo buffer;
			BufferType buffer_type;
			uint64_t size;
		};

		BufferManager();
		~BufferManager();

		void allocate_segment(BufferType buff_type, BufferMemoryType mem_type, uint64_t block_size);

		Buffer create(uint64_t size);

	private:

		vk::Device device;

		std::unordered_map<BufferMemoryType, std::vector<BufferSegmentInfo> > buffer_segments;
	};

}

