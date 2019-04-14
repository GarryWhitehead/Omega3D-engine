#pragma once
#include "Vulkan/Common.h"
#include "Vulkan/MemoryAllocator.h"

#include "Managers/EventManager.h"

#include <unordered_map>

namespace VulkanAPI
{
	namespace Util
	{
		uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags, vk::PhysicalDevice gpu);
	}

	// forward decleartions
	class DescriptorSet;

	struct BufferUpdateEvent : public OmegaEngine::Event
	{
		BufferUpdateEvent(const char* _id, void* _data, uint32_t _size, MemoryUsage _usage) :
			id(_id),
			data(_data),
			size(_size),
			mem_type(_usage)
		{}

		const char* id;
		void* data = nullptr;
		uint32_t size = 0;
		MemoryUsage mem_type;
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

		BufferManager(vk::Device dev, vk::PhysicalDevice phys_dev, vk::Queue qeuue);
		~BufferManager();

		void enqueueDescrUpdate(DescrSetUpdateInfo& descr_update);
		void enqueueDescrUpdate(const char *id, DescriptorSet* set, uint32_t set_num, uint32_t binding, vk::DescriptorType descr_type);

		void update_descriptors();

		void update_buffer(BufferUpdateEvent& event);

		// returns a wrapper containing vulkan memory buffer information
		Buffer get_buffer(const char* id);

	private:

		// local vulkan instance
		vk::Device device;
		vk::PhysicalDevice gpu;
		vk::Queue graph_queue;

		std::unique_ptr<MemoryAllocator> memory_allocator;

		std::unordered_map<const char *, MemorySegment> buffers;

		// a queue of descriptor sets which need updating this frame
		std::vector<DescrSetUpdateInfo> descr_set_update_queue;
	};

}

