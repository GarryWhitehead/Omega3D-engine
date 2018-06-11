#pragma once
#include "VulkanCore/vulkan_utility.h"

enum class DependencyTemplate
{
	TEMPLATE_TOP_OF_PIPE,
	TEMPLATE_BOTTOM_OF_PIPE,
	TEMPLATE_MULTI_SUBPASS
};

class VulkanRenderPass
{

public:
	VulkanRenderPass();
	~VulkanRenderPass();

	void AddAttachment(const VkImageLayout finalLayout, const VkFormat format);
	void AddSubPass(std::vector<VkAttachmentReference>& colorRef, std::vector<VkAttachmentReference>& inputRef, VkAttachmentReference *depthRef = nullptr);
	void AddSubPass(std::vector<VkAttachmentReference>& colorRef, VkAttachmentReference *depthRef = nullptr);																		// override without input attachments
	void AddSubpassDependency(DependencyTemplate depend_template, uint32_t srcSubpass = 0, uint32_t dstSubpass = 0);											// templated version
	void AddReference(const VkImageLayout layout, const uint32_t attachId);
	void PrepareRenderPass(const VkDevice device);
	void PrepareFramebuffer(const VkImageView imageView, uint32_t width, uint32_t height, const VkDevice device, uint32_t layerCount = 1);
	void PrepareFramebuffer(std::vector<VkImageView>& imageView, uint32_t width, uint32_t height, const VkDevice device, uint32_t layerCount = 1);


	VkRenderPass renderpass;
	VkFramebuffer frameBuffer;
	std::vector<VkAttachmentDescription> attachment;
	std::vector<VkAttachmentReference> colorReference;
	std::vector<VkAttachmentReference> depthReference;
	std::vector<VkSubpassDescription> subpass;
	std::vector<VkSubpassDependency> dependency;
};

