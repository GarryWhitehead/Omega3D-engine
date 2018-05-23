#pragma once
#include "VulkanCore/vulkan_utility.h"
class VulkanRenderPass
{

public:
	VulkanRenderPass();
	~VulkanRenderPass();

	void AddAttachment(const VkImageLayout finalLayout, const VkFormat format);
	void AddReference(const VkImageLayout layout, const uint32_t attachId);
	void PrepareRenderPass(const VkDevice device);
	void PrepareFramebuffer(const VkImageView imageView, uint32_t width, uint32_t height, const VkDevice device, uint32_t layerCount = 1);


	VkRenderPass renderpass;
	VkFramebuffer frameBuffer;
	std::vector<VkAttachmentDescription> attachment;
	std::vector<VkAttachmentReference> colorReference;
	std::vector<VkAttachmentReference> depthReference;

};

