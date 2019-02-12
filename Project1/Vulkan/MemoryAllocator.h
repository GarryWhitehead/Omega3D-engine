#pragma once
#include "Vulkan/Common.h"

#include <vector>
#include <unordered_map>

namespace VulkanAPI
{
	class MemoryAllocator;

	enum class MemoryType
	{
		VK_BLOCK_TYPE_HOST,
		VK_BLOCK_TYPE_LOCAL
	};

	enum class MemoryUsage
	{
		VK_UNIFORM_BUFFER = 1 << 0,
		VK_STORAGE_BUFFER = 1 << 1,
		VK_VERTEX_BUFFER = 1 << 2,
		VK_INDEX_BUFFER = 1 << 3,
		VK_BUFFER_STATIC = 1 << 4,		
		VK_BUFFER_DYNAMIC = 1 << 5
	};

	inline bool operator& (MemoryUsage a, MemoryUsage b)
	{
		return static_cast<int>(a) & static_cast<int>(b);
	}

	inline MemoryUsage operator| (MemoryUsage a, MemoryUsage b)
	{
		return static_cast<MemoryUsage>(static_cast<int>(a) | static_cast<int>(b));
	}

	class DynamicSegment
	{
	public:

		DynamicSegment() {}
		DynamicSegment(uint32_t size, uint32_t buffer_index);
		~DynamicSegment();

		uint32_t get_alignment_size() const
		{
			return alignment_size;
		}

		int32_t get_buffer_index() const
		{
			assert(buffer_index > 0);
			return buffer_index;
		}

		bool add_object(uint32_t max)
		{
			++objects_allocated;
			if (objects_allocated > max) {
				return false;
			}
			return true;
		}

	private:

		// how many objects allocated - if full,  then at present fails (TODO: Dont fail!!)
		uint32_t objects_allocated = 0;

		uint32_t alignment_size = 0;
		int32_t buffer_index = -1;
	};


	class MemorySegment
	{
	public:

		MemorySegment() {}

		MemorySegment(uint32_t id, uint32_t os, uint32_t sz) :
			block_id(id),
			offset(os),
			size(sz)
		{}

		uint32_t get_offset() const
		{
			return offset;
		}

		uint32_t get_size() const
		{
			return size;
		}

		int32_t get_id() const
		{
			return block_id;
		}

		// memory mapping functions
		void map(vk::Device dev, vk::DeviceMemory memory, const uint32_t offset, void* data, uint32_t totalSize, uint32_t mapped_offset);		// for data types of *void

		template <typename T>
		void map(vk::Device dev, vk::DeviceMemory memory, const uint32_t offset, std::vector<T>& data_src)
		{
			uint32_t data_size = sizeof(T) * data.size();
			assert(data_size <= size);

			if (data == nullptr) {
				VK_CHECK_RESULT(dev.mapMemory(memory, offset, size, 0, &data));
				memcpy(data, data_src.data(), data_size);
				dev.unmapMemory(memory);
			}
			else {
				memcpy(data, data_src.data(), data_size);
			}
		}

	private:

		int32_t block_id;			// index into memory block container
		uint32_t offset;
		uint32_t size;
		void *data = nullptr;
	};

	class MemoryAllocator
	{

	public:

		// default block size - can be overriden by user
		static constexpr float ALLOC_BLOCK_SIZE_LOCAL = 2.56e+8f;			// each default device local block is 256mb
		static constexpr float ALLOC_BLOCK_SIZE_HOST = 6.4e+7f;				// host block deaults to 64mb

		MemoryAllocator();
		~MemoryAllocator();

		// no copy assignment allowed
		MemoryAllocator(const MemoryAllocator&) = delete;
		MemoryAllocator& operator=(const MemoryAllocator&) = delete;

		void init(vk::Device dev, vk::PhysicalDevice physical);

		// helper functions
		void createBuffer(uint32_t size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props, vk::DeviceMemory& memory, vk::Buffer& buffer);
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags);
		vk::Buffer& get_memory_buffer(const uint32_t id);

		// Segment allocation functions and mapping
		MemorySegment allocate(MemoryUsage usage, vk::BufferUsageFlagBits buffer_usage, uint32_t size);
		void mapDataToSegment(MemorySegment &segment, void *data, uint32_t totalSize, uint32_t offset = 0);
		
		// dynamic buffer allocation
		DynamicSegment* allocate_dynamic(uint32_t size, uint32_t objects);
		void mapDataToDynamicSegment(DynamicSegment* segment, void *data, uint32_t totalSize, uint32_t offset = 0);
		
	private:

		struct MemoryBlock
		{
			int32_t block_id = -1;

			MemoryType type;
			uint32_t total_size = 0;

			std::unordered_map<uint32_t, uint32_t> alloc_segments;
			std::unordered_map<uint32_t, uint32_t> free_segments;		// fisrt = offset - second = size

			// vulkan info
			vk::DeviceMemory block_mem;
			vk::Buffer block_buffer;
		};

		// Block allocation functions
		uint32_t allocateBlock(MemoryType type, vk::BufferUsageFlagBits usage, uint32_t size = 0);
		uint32_t allocateBlock(MemoryUsage usage, vk::BufferUsageFlagBits buffer_usage);
		void destroyBlock(uint32_t block_id);
		void destroyAllBlocks();

		// segment functions
		void destroySegment(MemorySegment &segment);
		uint32_t findBlockType(MemoryUsage usage);
		uint32_t findFreeSegment(uint32_t block_id, uint32_t size);

		// the current device 
		vk::Device device;
		vk::PhysicalDevice gpu;

		std::vector<MemoryBlock> mem_blocks;

		// dynamic buffer - there is a limit on the number of dynamic buffers vulkan allows
		std::vector<MemorySegment> dynamic_buffers;
	};

	
}
