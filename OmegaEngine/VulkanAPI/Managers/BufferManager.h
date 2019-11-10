#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/Managers/MemoryAllocator.h"

#include "utility/String.h"

#include <unordered_map>

// forward declerations
namespace OmegaEngine
{
class UBufferUpdateInfo;
class UBufferCreateInfo;
}

namespace VulkanAPI
{
namespace VulkanUtil
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
class BufferReflect;

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
	
	struct BufferWrapper
	{
		vk::Buffer buffer;
		uint32_t offset;
	};

	BufferManager();
	~BufferManager();

	void init(VkContext* con);

	void BufferManager::prepareDescrSet(BufferReflect& reflected, DescriptorSet& descrSet);

	bool updateBuffers(std::vector<OmegaEngine::UBufferUpdateInfo>& updates);
	bool createBuffers(std::vector<OmegaEngine::UBufferCreateInfo>& updates);
	
	// returns a wrapper containing vulkan memory buffer information
	BufferWrapper getBuffer(Util::String id);

private:
	// local vulkan instance
	VkContext* context;

	std::unique_ptr<MemoryAllocator> memoryAllocator;

	std::unordered_map<Util::String, MemorySegment> buffers;

};

} // namespace VulkanAPI
