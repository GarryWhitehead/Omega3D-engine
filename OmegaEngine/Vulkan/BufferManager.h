#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/MemoryAllocator.h"

#include "Managers/EventManager.h"

#include <unordered_map>

namespace VulkanAPI
{
	namespace Util
	{
		void createBuffer(vk::Device& device, vk::PhysicalDevice& gpu, const uint32_t size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props, vk::DeviceMemory& memory, vk::Buffer& buffer);
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags, vk::PhysicalDevice gpu);
		uint32_t alignment_size(const uint32_t size);
	}

	// forward decleartions
	class DescriptorSet;

	struct BufferUpdateEvent : public OmegaEngine::Event
	{
		BufferUpdateEvent(const char* _id, void* _data, uint64_t _size, MemoryUsage _usage) :
			id(_id),
			data(_data),
			size(_size),
			mem_type(_usage)
		{}

		BufferUpdateEvent(const char* _id, void* _data, uint64_t _size, MemoryUsage _usage, bool flush) :
			id(_id),
			data(_data),
			size(_size),
			mem_type(_usage),
			flush_memory(flush)
		{}

		BufferUpdateEvent() {}

		const char* id;
		void* data = nullptr;
		uint64_t size = 0;
		MemoryUsage mem_type;
		bool flush_memory = false;
	};

	struct Buffer
	{
		vk::Buffer buffer;
		uint32_t offset;
	};

	class BufferManager
	{
	public:

		struct DescrSetUpdateInfo
		{
			const char *id;
			DescriptorSet* set;
			uint32_t set_num = 0;
			uint32_t binding = 0;
			vk::DescriptorType descr_type;
		};

		BufferManager(vk::Device dev, vk::PhysicalDevice phys_dev, Queue qeuue);
		~BufferManager();

		void enqueueDescrUpdate(DescrSetUpdateInfo& descr_update);
		void enqueueDescrUpdate(const char *id, DescriptorSet* set, uint32_t set_num, uint32_t binding, vk::DescriptorType descr_type);

		void update();
		void update_descriptors();
		void update_buffer(BufferUpdateEvent& event);

		// returns a wrapper containing vulkan memory buffer information
		Buffer getBuffer(const char* id);

	private:

		// local vulkan instance
		vk::Device device;
		vk::PhysicalDevice gpu;
		Queue graph_queue;

		std::unique_ptr<MemoryAllocator> memory_allocator;

		std::unordered_map<const char *, MemorySegment> buffers;

		// a queue of descriptor sets which need updating this frame
		std::vector<DescrSetUpdateInfo> descriptorSet_update_queue;
	};

}

