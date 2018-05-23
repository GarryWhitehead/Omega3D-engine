#pragma once
#include "VulkanCore/vulkan_utility.h"

class VulkanBuffer
{
public:

	VulkanBuffer();
	~VulkanBuffer();

	void CreateBuffer(const uint32_t size, const VkBufferUsageFlags flags, const VkMemoryPropertyFlags props, VulkanEngine *p_vkEngine);

	template <typename T>
	void MapBuffer(std::vector<T>& bufferData, VkDevice device);

	template <typename T>
	void MapBuffer(T bufferData, VkDevice device);

	void *mappedData;
	VkBuffer buffer;
	VkDeviceMemory memory;
	uint32_t size;
	uint32_t offset;

};

template <typename T>
void VulkanBuffer::MapBuffer(std::vector<T>& bufferData, VkDevice device)
{
	assert(bufferData.size() != 0);
	assert(bufferData.size() * sizeof(T) <= size);
	if (mappedData == nullptr) {
		VK_CHECK_RESULT(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mappedData));
		memcpy(mappedData, bufferData.data(), bufferData.size() * sizeof(T));
		vkUnmapMemory(device, memory);
	}
	else {
		memcpy(mappedData, bufferData.data(), bufferData.size() * sizeof(T));
	}
}

template <typename T>
void VulkanBuffer::MapBuffer(T bufferData, VkDevice device)
{
	if (mappedData == nullptr) {
		VK_CHECK_RESULT(vkMapMemory(device, memory, 0, VK_WHOLE_SIZE, 0, &mappedData));
		memcpy(mappedData, &bufferData, sizeof(T));
		vkUnmapMemory(device, memory);
	}
	else {
		memcpy(mappedData, &bufferData, sizeof(T));
	}
}

