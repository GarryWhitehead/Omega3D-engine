#include "VulkanRenderPass.h"



VulkanRenderPass::VulkanRenderPass()
{
}


VulkanRenderPass::~VulkanRenderPass()
{

}

void VulkanRenderPass::AddAttachment(const VkImageLayout finalLayout, const VkFormat format)
{
	VkAttachmentDescription attachDescr = {};
	attachDescr.format = format;
	attachDescr.samples = VK_SAMPLE_COUNT_1_BIT;			// used for MSAA
	attachDescr.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDescr.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// IMPORTANT: this needs to be set to store operations for this to work!!!
	attachDescr.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDescr.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDescr.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDescr.finalLayout = finalLayout;

	attachment.push_back(attachDescr);
}

void VulkanRenderPass::AddReference(const VkImageLayout layout, const uint32_t attachId)
{
	VkAttachmentReference ref = {};
	
	if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		ref.attachment = attachId;
		ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthReference.push_back(ref);
	}
	else {
		ref.attachment = attachId;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorReference.push_back(ref);
	}
}

void VulkanRenderPass::PrepareRenderPass(const VkDevice device)
{
	assert(!colorReference.empty() || !depthReference.empty());

	std::array<VkSubpassDependency, 2> sPassDepend = {};
	sPassDepend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[0].dstSubpass = 0;
	sPassDepend[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	sPassDepend[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[1].srcSubpass = 0;
	sPassDepend[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	sPassDepend[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkSubpassDescription sPassDescr = {};
	sPassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	if (!colorReference.empty()) {
		sPassDescr.colorAttachmentCount = static_cast<uint32_t>(colorReference.size());
		sPassDescr.pColorAttachments = colorReference.data();
	}
	if (!depthReference.empty()) {
		sPassDescr.pDepthStencilAttachment = depthReference.data();
	}

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
	createInfo.pAttachments = attachment.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sPassDescr;
	createInfo.dependencyCount = static_cast<uint32_t>(sPassDepend.size());
	createInfo.pDependencies = sPassDepend.data();

	VK_CHECK_RESULT(vkCreateRenderPass(device, &createInfo, nullptr, &renderpass));
}

void VulkanRenderPass::PrepareFramebuffer(const VkImageView imageView, uint32_t width, uint32_t height, const VkDevice device, uint32_t layerCount)
{
	assert(imageView != VK_NULL_HANDLE);
	assert(renderpass != VK_NULL_HANDLE);

	VkFramebufferCreateInfo frameInfo = {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = renderpass;
	frameInfo.attachmentCount = 1;
	frameInfo.pAttachments = &imageView;
	frameInfo.width = width;
	frameInfo.height = height;
	frameInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameInfo, nullptr, &frameBuffer))
}

