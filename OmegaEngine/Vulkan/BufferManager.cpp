#include "BufferManager.h"
#include "utility/logger.h"
#include "Vulkan/Descriptors.h"

#include "Engine/Omega_Global.h"


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


	BufferManager::BufferManager(vk::Device dev, vk::PhysicalDevice phys_dev, vk::Queue queue) :
		device(dev),
		gpu(phys_dev),
		graph_queue(queue)
	{
		OmegaEngine::Global::eventManager()->registerListener<BufferManager, BufferUpdateEvent, &BufferManager::update_buffer>(this);
		
	}

	BufferManager::~BufferManager()
	{
	}

	void BufferManager::enqueueDescrUpdate(DescrSetUpdateInfo& descr_update)
	{
		descr_set_update_queue.emplace_back(descr_update);
	}

	void BufferManager::enqueueDescrUpdate(const char *id, DescriptorSet* set, uint32_t set_num, uint32_t binding, vk::DescriptorType descr_type)
	{
		descr_set_update_queue.push_back({ id, set, set_num, binding, descr_type });
	}

	void BufferManager::update_buffer(BufferUpdateEvent& event)
	{
		assert(event.data != nullptr);
		
		MemorySegment buffer;

		// check that the maanger doesn't already contain the same id - if it does then update the buffer
		if (buffers.find(event.id) != buffers.end()) {

			buffer = buffers[event.id];
			memory_allocator->mapDataToSegment(buffer, event.data, event.size);
		}
		else {
			// otherwise create a new memory segment
			MemorySegment buffer = memory_allocator->allocate(event.mem_type, event.size);
			memory_allocator->mapDataToSegment(buffer, event.data, event.size);
			buffers[event.id] = buffer;
		}
	}

	void BufferManager::update_descriptors()
	{
	
		if (!descr_set_update_queue.empty()) {

			for (auto& descr : descr_set_update_queue) {

				if (buffers.find(descr.id) == buffers.end()) {

					LOGGER_ERROR("Buffer manager has no buffer with id: %s\n", descr.id);
				}

				MemorySegment segment = buffers[descr.id];

				if (descr.descr_type == vk::DescriptorType::eCombinedImageSampler) {

				}
				else {
					descr.set->write_set(descr.set_num, descr.binding, descr.descr_type, memory_allocator->get_memory_buffer(segment.get_id()), segment.get_offset(), segment.get_size());
				}
			
			}
		}
	}

	Buffer BufferManager::get_buffer(const char* id)
	{
		if (buffers.find(id) == buffers.end()) {
			LOGGER_ERROR("Error trying to find buffer id. Unable to proceed.\n");
		}

		return { memory_allocator->get_memory_buffer(buffers[id].get_id()), buffers[id].get_offset() };
	}
}