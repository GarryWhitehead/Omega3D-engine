#include "BufferManager.h"
#include "VulkanAPI/Descriptors.h"
#include "utility/logger.h"

#include "Engine/Omega_Global.h"

namespace VulkanAPI
{
namespace Util
{
// static functions
uint32_t alignmentSize(const uint32_t size)
{
	// we are presuming the min alignment size here so we don't have to use the vulkan api within the peripheral managers
	uint32_t min_align = 256;
	return (size + min_align - 1) & ~(min_align - 1);
}

uint32_t findMemoryType(const uint32_t type, const vk::MemoryPropertyFlags flags,
                        vk::PhysicalDevice gpu)
{
	vk::PhysicalDeviceMemoryProperties memoryProp = gpu.getMemoryProperties();

	for (uint32_t c = 0; c < memoryProp.memoryTypeCount; ++c)
	{
		if ((type & (1 << c)) && (memoryProp.memoryTypes[c].propertyFlags & flags) == flags)
			return c;
	}

	return UINT32_MAX;
}

void createBuffer(vk::Device &device, vk::PhysicalDevice &gpu, const uint32_t size,
                  vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props,
                  vk::DeviceMemory &memory, vk::Buffer &buffer)
{
	vk::BufferCreateInfo createInfo({}, size, flags, vk::SharingMode::eExclusive);

	VK_CHECK_RESULT(device.createBuffer(&createInfo, nullptr, &buffer));

	vk::MemoryRequirements memoryReq;
	device.getBufferMemoryRequirements(buffer, &memoryReq);

	uint32_t memoryType = findMemoryType(memoryReq.memoryTypeBits, props, gpu);

	vk::MemoryAllocateInfo memoryInfo(memoryReq.size, memoryType);

	VK_CHECK_RESULT(device.allocateMemory(&memoryInfo, nullptr, &memory));
	device.bindBufferMemory(buffer, memory, 0);
}
} // namespace Util

BufferManager::BufferManager(vk::Device dev, vk::PhysicalDevice physicalDevice, Queue queue)
    : device(dev)
    , gpu(physicalDevice)
    , graphicsQueue(queue)
{
	OmegaEngine::Global::eventManager()
	    ->registerListener<BufferManager, BufferUpdateEvent, &BufferManager::updateBuffer>(this);

	memoryAllocator = std::make_unique<MemoryAllocator>(device, gpu, graphicsQueue);
}

BufferManager::~BufferManager()
{
}

void BufferManager::enqueueDescrUpdate(DescrSetUpdateInfo &descriptorUpdate)
{
	descriptorSetUpdateQueue.emplace_back(descriptorUpdate);
}

void BufferManager::enqueueDescrUpdate(const char *id, DescriptorSet *set, uint32_t setValue,
                                       uint32_t binding, vk::DescriptorType descriptorType)
{
	descriptorSetUpdateQueue.push_back({ id, set, setValue, binding, descriptorType });
}

void BufferManager::updateBuffer(BufferUpdateEvent &event)
{
	// sanity debugging checks
	assert(event.data != nullptr);
	assert(event.size > 0);

	MemorySegment buffer;

	auto iter = buffers.begin();
	while (iter != buffers.end())
	{

		if (std::strcmp(iter->first, event.id) == 0)
		{
			break;
		}
		iter++;
	}

	// check that the maanger doesn't already contain the same id - if it does then update the buffer
	if (iter != buffers.end())
	{

		buffer = iter->second;
		memoryAllocator->mapDataToSegment(buffer, event.data, event.size);

		if (event.flushMemory)
		{
			vk::MappedMemoryRange mem_range(memoryAllocator->getDeviceMemory(buffer.getId()),
			                                (uint64_t)buffer.getOffset(), event.size);
			device.flushMappedMemoryRanges(1, &mem_range);
		}
	}
	else
	{
		// otherwise create a new memory segment
		MemorySegment buffer = memoryAllocator->allocate(event.memoryType, event.size);
		memoryAllocator->mapDataToSegment(buffer, event.data, event.size);
		buffers[event.id] = buffer;
	}
}

void BufferManager::updateDescriptors()
{

	if (!descriptorSetUpdateQueue.empty())
	{

		for (auto &descr : descriptorSetUpdateQueue)
		{

			auto iter = buffers.begin();
			while (iter != buffers.end())
			{

				if (std::strcmp(iter->first, descr.id) == 0)
				{
					break;
				}
				iter++;
			}

			if (iter != buffers.end())
			{
				// this may not potentially be an error. For instance, the skinned program state is set up though there is no data
				// so the descriptors won't be updated.
				MemorySegment segment = iter->second;
				descr.set->writeSet(descr.setValue, descr.binding, descr.descriptorType,
				                    memoryAllocator->getMemoryBuffer(segment.getId()),
				                    segment.getOffset(), segment.getSize());
			}
		}
	}

	descriptorSetUpdateQueue.clear();
}

void BufferManager::update()
{
	updateDescriptors();
}

Buffer BufferManager::getBuffer(const char *id)
{
	auto iter = buffers.begin();
	while (iter != buffers.end())
	{

		if (std::strcmp(iter->first, id) == 0)
		{
			break;
		}
		iter++;
	}

	if (iter == buffers.end())
	{
		LOGGER_ERROR("Error. Unable to find id: %s within buffer list", id);
	}
	return { memoryAllocator->getMemoryBuffer(iter->second.getId()), iter->second.getOffset() };
}
} // namespace VulkanAPI