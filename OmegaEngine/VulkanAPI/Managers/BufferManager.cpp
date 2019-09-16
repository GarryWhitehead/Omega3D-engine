#include "BufferManager.h"

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/VkContext.h"

#include "VulkanAPI/Types/BufferReflect.h"

#include "utility/logger.h"

#include "Types/UBufferInfo.h"

#include "Core/Omega_Global.h"

namespace VulkanAPI
{
namespace VulkanUtil
{
// static functions
uint32_t alignmentSize(const uint32_t size)
{
	// we are presuming the min alignment size here so we don't have to use the vulkan api within the peripheral managers
	uint32_t min_align = 256;
	return (size + min_align - 1) & ~(min_align - 1);
}

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

void createBuffer(VkContext* context, const uint32_t size, vk::BufferUsageFlags flags, vk::MemoryPropertyFlags props,
                  vk::DeviceMemory& memory, vk::Buffer& buffer)
{
	vk::BufferCreateInfo createInfo({}, size, flags, vk::SharingMode::eExclusive);

	VK_CHECK_RESULT(context->getDevice().createBuffer(&createInfo, nullptr, &buffer));

	vk::MemoryRequirements memoryReq;
	context->getDevice().getBufferMemoryRequirements(buffer, &memoryReq);

	uint32_t memoryType = findMemoryType(memoryReq.memoryTypeBits, props, context->getGpu());

	vk::MemoryAllocateInfo memoryInfo(memoryReq.size, memoryType);

	VK_CHECK_RESULT(device.allocateMemory(&memoryInfo, nullptr, &memory));
	context->getDevice().bindBufferMemory(buffer, memory, 0);
}
}    // namespace VulkanUtil

BufferManager::BufferManager()
{
}

BufferManager::~BufferManager()
{
}

void BufferManager::init(VkContext* con)
{
	this->context = con;

	memoryAllocator = std::make_unique<MemoryAllocator>(context->getDevice(), context->getGpu(),
	                                                    context->getQueue(VkContext::QueueType::Graphics));
}

bool BufferManager::updateBuffers(std::vector<OmegaEngine::UBufferUpdateInfo>& updates)
{
	if (updates.empty())
	{
		return true;
	}

	for (auto& update : updates)
	{
		// sanity debugging checks
		assert(update.data);
		assert(update.size > 0);

		// check that the maanger contains the buffer id
		auto iter = buffers.find(update.id);	
		if (iter == buffers.end())
		{
			LOGGER_ERROR("Trying to update buffer with id: %s but it doesn't exsist. Are you sure you created it?\n", update.id.c_str());
			return false;
		}

		MemorySegment buffer = iter->second;
		memoryAllocator->mapDataToSegment(buffer, update.data, update.size);

		// Note: this should only be done for non-coherent memory
		if (update.flushMem)
		{
			vk::MappedMemoryRange mem_range(memoryAllocator->getDeviceMemory(buffer.getId()),
											(uint64_t)buffer.getOffset(), update.size);
			context->getDevice().flushMappedMemoryRanges(1, &mem_range);
		}
	}
}

bool BufferManager::createBuffers(std::vector<OmegaEngine::UBufferCreateInfo>& newBuffers)
{
	if (newBuffers.empty())
	{
		return;
	}

	for (auto& newBuffer : newBuffers)
	{
		// sanity debugging checks
		assert(newBuffer.data);
		assert(newBuffer.size > 0);

		// check that a buffer with the same id doesn't already exsist
		auto iter = buffers.find(newBuffer.id);
		if (iter != buffers.end())
		{
			LOGGER_ERROR("Buffer with id: %s already exsists within manager.\n", newBuffer.id.c_str());
			return false;
		}

		// otherwise create a new memory segment
		MemorySegment buffer = memoryAllocator->allocate(newBuffer.memoryType, newBuffer.size);
		memoryAllocator->mapDataToSegment(buffer, newBuffer.data, newBuffer.size);
		buffers[newBuffer.id] = buffer;
	}

	return true;
}

void BufferManager::prepareDescrSet(BufferReflect& reflected, DescriptorSet& descrSet)
{

	if (reflected.layouts.empty())
	{
		return;
	}

	for (auto& layout : reflected.layouts)
	{
		// strip down ubo name to obtain the id used to assocaite the buffer
		// for instance: the buffer used to create the CameraUbo data will be "Camera"
		// Though the ubo title can also contain additional information designated by
		// a underscore before the buffer title proper. For instance: Dyanamic_CameraUbo
		// states that this is a dynamic buffer. This needs to be stripped to start.
		Util::String str;

		auto& splitStr = layout.name.split('_');
		assert(splitStr.size() <= 1);

		// if we only have one string, then no pre-descriptor
		if (splitStr.size == 1)
		{
			str = splitStr[0];
		}
		else
		{
			str = splitStr[1];
		}

		auto iter = buffers.find(str);

		if (iter != buffers.end())
		{
			// this may not potentially be an error. For instance, the skinned program state is set up though there is no data
			// so the descriptors won't be updated.
			MemorySegment segment = iter->second;
			descrSet.writeSet(layout.set, layout.binding, layout.type,
			                    memoryAllocator->getMemoryBuffer(segment.getId()), segment.getOffset(),
			                    segment.getSize());
		}
	}
}

BufferManager::BufferWrapper BufferManager::getBuffer(Util::String id)
{
	auto iter = buffers.find(id);

	if (iter == buffers.end())
	{
		LOGGER_ERROR("Error. Unable to find id: %s within buffer list", id);
	}
	return { memoryAllocator->getMemoryBuffer(iter->second.getId()), iter->second.getOffset() };
}
}    // namespace VulkanAPI
