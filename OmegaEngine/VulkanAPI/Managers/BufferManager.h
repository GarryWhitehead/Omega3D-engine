#pragma once
#include "VulkanAPI/Common.h"

#include "VulkanAPI/Managers/MemoryAllocator.h"

#include "Managers/EventManager.h"

#include <unordered_map>

namespace VulkanAPI
{
namespace Util
{
void createBuffer(vk::Device &device, vk::PhysicalDevice &gpu, const uint32_t size,
                  vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props,
                  vk::DeviceMemory &memory, vk::Buffer &buffer);
uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags,
                        vk::PhysicalDevice gpu);
uint32_t alignmentSize(const uint32_t size);
} // namespace Util

// forward decleartions
class DescriptorSet;
class VkContext;

struct BufferUpdateEvent : public OmegaEngine::Event
{
	BufferUpdateEvent(const char *_id, void *_data, uint64_t _size, MemoryUsage _usage)
	    : id(_id)
	    , data(_data)
	    , size(_size)
	    , memoryType(_usage)
	{
	}

	BufferUpdateEvent(const char *_id, void *_data, uint64_t _size, MemoryUsage _usage, bool flush)
	    : id(_id)
	    , data(_data)
	    , size(_size)
	    , memoryType(_usage)
	    , flushMemory(flush)
	{
	}

	BufferUpdateEvent()
	{
	}

	const char *id;
	void *data = nullptr;
	uint64_t size = 0;
	MemoryUsage memoryType;
	bool flushMemory = false;
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
		DescriptorSet *set;
		uint32_t setValue = 0;
		uint32_t binding = 0;
		vk::DescriptorType descriptorType;
	};

	BufferManager();
	~BufferManager();

	void enqueueDescrUpdate(DescrSetUpdateInfo &descriptorUpdate);
	void enqueueDescrUpdate(const char *id, DescriptorSet *set, uint32_t setValue, uint32_t binding,
	                        vk::DescriptorType descriptorType);

	void init(VkContext* con);
	void update();
	void updateDescriptors();
	void updateBuffer(BufferUpdateEvent &event);

	// returns a wrapper containing vulkan memory buffer information
	Buffer getBuffer(const char *id);

private:
	// local vulkan instance
	VkContext* context;

	std::unique_ptr<MemoryAllocator> memoryAllocator;

	std::unordered_map<const char *, MemorySegment> buffers;

	// a queue of descriptor sets which need updating this frame
	std::vector<DescrSetUpdateInfo> descriptorSetUpdateQueue;
};

} // namespace VulkanAPI
