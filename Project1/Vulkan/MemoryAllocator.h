#pragma once
#include <vector>
#include <unordered_map>
#include "Vulkan/Common.h"

namespace VulkanAPI
{

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

	class MemoryAllocator
	{

	public:

		// default block size - can be overriden by user
		static constexpr float ALLOC_BLOCK_SIZE_LOCAL = 2.56e+8f;			// each default device local block is 256mb
		static constexpr float ALLOC_BLOCK_SIZE_HOST = 6.4e+7f;				// host block deaults to 64mb

		struct SegmentInfo
		{
			SegmentInfo() : data(nullptr), block_id(0), offset(0), size(0) {}

			SegmentInfo(uint32_t id, uint32_t os, uint32_t sz) :
				block_id(id),
				offset(os),
				size(sz),
				data(nullptr)
			{}

			int32_t block_id;			// index into memory block container
			uint32_t offset;
			uint32_t size;
			void *data;

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

		};

		struct BlockInfo
		{
			int32_t block_id;

			MemoryType type;
			uint32_t total_size;

			std::unordered_map<uint32_t, uint32_t> alloc_segments;
			std::unordered_map<uint32_t, uint32_t> free_segments;		// fisrt = offset - second = size

			// vulkan info
			vk::DeviceMemory block_mem;
			vk::Buffer block_buffer;
		};

		MemoryAllocator(vk::Device dev, vk::PhysicalDevice physical);
		~MemoryAllocator();

		// no copy assignment allowed
		MemoryAllocator(const MemoryAllocator&) = delete;
		MemoryAllocator& operator=(const MemoryAllocator&) = delete;

		// Block allocation functions
		uint32_t allocateBlock(MemoryType type, uint32_t size = 0);
		uint32_t allocateBlock(MemoryUsage usage);
		void destroyBlock(uint32_t block_id);
		void destroyAllBlocks();

		// Segment allocation functions
		SegmentInfo allocateSegment(MemoryUsage usage, uint32_t size);
		void destroySegment(SegmentInfo &segment);
		uint32_t findFreeSegment(uint32_t block_id, uint32_t size);

		// helper functions
		void createBuffer(const uint32_t size, const vk::BufferUsageFlags flags, const vk::MemoryPropertyFlags props, vk::DeviceMemory& memory, vk::Buffer& buffer);
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags);
		uint32_t findBlockType(MemoryUsage usage);
		//void defragBlockMemory(const uint32_t id);
		VkBuffer& blockBuffer(const uint32_t id);

		// memory mapping functions
		void MemoryAllocator::mapDataToSegment(SegmentInfo &segment, void *data, uint32_t totalSize, uint32_t offset = 0);


		// for data stored in containers
		template <typename T>
		void mapDataToSegment(SegmentInfo &segment, std::vector<T> data)
		{
			assert(segment.block_id < (int32_t)mem_blocks.size());
			BlockInfo block = mem_blocks[segment.block_id];

			// How we map the data depends on the memory type; 
			// For host-visible memory, the memory is mapped and the data is just mem-copied across
			// With device local, we must first create a buffer on the host, map the data to that and then copyCmd it across to local memory

			if (block.type == MemoryType::VK_BLOCK_TYPE_HOST) {

				segment.map(device, block.block_mem, segment.offset, data);
			}
			else if (block.type == MemoryType::VK_BLOCK_TYPE_LOCAL) {

				// start by creating host-visible buffers
				vk::Buffer temp_buffer;
				vk::DeviceMemory temp_memory;
				CreateBuffer(segment.size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, temp_memory, temp_buffer);

				// map data to temporary buffer
				segment.map(device, temp_memory, 0, data);

				// create cmd buffer for copy and transfer to device local memory
				VulkanUtility::CopyBuffer(temp_buffer, block.block_buffer, segment.size, p_vkEngine->GetCmdPool(), VulkanGlobal::vkCurrent.graphQueue, VulkanGlobal::vkCurrent.device, 0, segment.offset);

				// clear up and we are done
				device.destroyBuffer(temp_buffer, nullptr);
				device.freeMemory(temp_memory, nullptr);
			}
		}

	private:

		// the current device 
		vk::Device device;
		vk::PhysicalDevice gpu;

		std::vector<BlockInfo> mem_blocks;
	};

	
}
