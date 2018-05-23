#include "VulkanBuffer.h"
#include "VulkanCore/VulkanEngine.h"


VulkanBuffer::VulkanBuffer() :
	mappedData(nullptr)
{
}


VulkanBuffer::~VulkanBuffer()
{
}

void VulkanBuffer::CreateBuffer(const uint32_t sz, const VkBufferUsageFlags flags, const VkMemoryPropertyFlags props, VulkanEngine *p_vkEngine)
{
	assert(sz > 0);
	size = sz;

	VulkanUtility *p_vkUtility = new VulkanUtility(p_vkEngine);

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = flags;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK_RESULT(vkCreateBuffer(p_vkEngine->GetDevice(), &createInfo, nullptr, &buffer));

	VkMemoryRequirements memoryReq;
	vkGetBufferMemoryRequirements(p_vkEngine->GetDevice(), buffer, &memoryReq);

	VkMemoryAllocateInfo memoryInfo = {};
	memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryInfo.allocationSize = memoryReq.size;
	memoryInfo.memoryTypeIndex = p_vkUtility->FindMemoryType(memoryReq.memoryTypeBits, props);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->GetDevice(), &memoryInfo, nullptr, &memory));

	VK_CHECK_RESULT(vkBindBufferMemory(p_vkEngine->GetDevice(), buffer, memory, 0));
}
