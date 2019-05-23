#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/Queue.h"

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
		VK_vertexBuffer = 1 << 2,
		VK_indexBuffer = 1 << 3,
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

	class MemorySegment
	{
	public:

		MemorySegment() {}

		MemorySegment(uint32_t id, uint32_t os, uint32_t sz) :
			blockId(id),
			offset(os),
			size(sz)
		{}

		uint32_t getOffset() const
		{
			return offset;
		}

		uint32_t getSize() const
		{
			return size;
		}

		int32_t getId() const
		{
			return blockId;
		}

		// memory mapping functions
		void map(vk::Device dev, vk::DeviceMemory memory, const uint32_t offset, void* data, uint32_t totalSize, uint32_t mappedOffset);		// for data types of *void

		template <typename T>
		void map(vk::Device dev, vk::DeviceMemory memory, const uint32_t offset, std::vector<T>& sourceData)
		{
			uint32_t dataSize = sizeof(T) * data.size();
			assert(dataSize <= size);

			if (data == nullptr) {
				VK_CHECK_RESULT(dev.mapMemory(memory, offset, size, 0, &data));
				memcpy(data, sourceData.data(), dataSize);
				dev.unmapMemory(memory);
			}
			else {
				memcpy(data, sourceData.data(), dataSize);
			}
		}

	private:

		int32_t blockId;			// index into memory block container
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
		MemoryAllocator(vk::Device& dev, vk::PhysicalDevice& physical, Queue& queue);
		~MemoryAllocator();

		// no copy assignment allowed
		MemoryAllocator(const MemoryAllocator&) = delete;
		MemoryAllocator& operator=(const MemoryAllocator&) = delete;

		void init(vk::Device& dev, vk::PhysicalDevice& physical, Queue& queue);

		// helper functions
		void createBuffer(uint32_t size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props, vk::DeviceMemory& memory, vk::Buffer& buffer);
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags);
		vk::Buffer& getMemoryBuffer(const uint32_t id);
		vk::DeviceMemory& getDeviceMemory(const uint32_t id);

		// Segment allocation functions and mapping
		MemorySegment allocate(MemoryUsage usage, uint32_t size);
		void mapDataToSegment(MemorySegment &segment, void *data, uint32_t totalSize, uint32_t offset = 0);

		// useful diagnostic functions
		void outputLog();
		
	private:

		struct MemoryBlock
		{
			int32_t blockId = -1;

			MemoryType type;
			uint32_t totalSize = 0;

			std::unordered_map<uint32_t, uint32_t> allocatedSegments;
			std::unordered_map<uint32_t, uint32_t> freeSegments;		// fisrt = offset - second = size

			// vulkan info
			vk::DeviceMemory blockMemory;
			vk::Buffer blockBuffer;
		};

		// Block allocation functions
		uint32_t allocateBlock(MemoryType type, uint32_t size);
		uint32_t allocateBlock(MemoryUsage usage);
		void destroyBlock(uint32_t blockId);
		void destroyAllBlocks();

		// segment functions
		void destroySegment(MemorySegment &segment);
		uint32_t findBlockType(MemoryUsage usage);
		uint32_t findFreeSegment(uint32_t blockId, uint32_t size);

		// the current device 
		vk::Device device;
		vk::PhysicalDevice gpu;
		Queue graphicsQueue;

		std::vector<MemoryBlock> memoryBlocks;
	};

	
}
