#include "VulkanRenderPass.h"



VulkanRenderPass::VulkanRenderPass(VkDevice dev) :
	frameBuffer(VK_NULL_HANDLE),
	device(dev)
{
	assert(device != VK_NULL_HANDLE);
}

VulkanRenderPass::VulkanRenderPass(VkDevice dev, VkRenderPass pass) :
	frameBuffer(VK_NULL_HANDLE),
	renderpass(pass),
	device(dev)
{
	assert(device != VK_NULL_HANDLE);
	assert(renderpass != VK_NULL_HANDLE);
}



VulkanRenderPass::~VulkanRenderPass()
{
	Destroy();
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

void VulkanRenderPass::AddSubPass(std::vector<VkAttachmentReference>& colorRef, std::vector<VkAttachmentReference>& inputRef, VkAttachmentReference *depthRef)
{
	assert(!colorRef.empty());
	assert(!inputRef.empty());
	
	// override default subpass with user specified subpass
	VkSubpassDescription subpassDescr = {};
	subpassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// colour attachements
	subpassDescr.colorAttachmentCount = static_cast<uint32_t>(colorRef.size());		
	subpassDescr.pColorAttachments = colorRef.data();

	// input attachments
	subpassDescr.inputAttachmentCount = static_cast<uint32_t>(inputRef.size());
	subpassDescr.pInputAttachments = inputRef.data();

	// depth attachment - if required
	if (depthRef != nullptr) {

		subpassDescr.pDepthStencilAttachment = depthRef;
	}
	
	subpass.push_back(subpassDescr);
}

void VulkanRenderPass::AddSubPass(std::vector<VkAttachmentReference>& colorRef, VkAttachmentReference *depthRef)
{
	assert(!colorRef.empty());

	// override default subpass with user specified subpass
	VkSubpassDescription subpassDescr = {};
	subpassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	// colour attachements
	subpassDescr.colorAttachmentCount = static_cast<uint32_t>(colorRef.size());
	subpassDescr.pColorAttachments = colorRef.data();

	// depth attachment - if required
	if (depthRef != nullptr) {

		subpassDescr.pDepthStencilAttachment = depthRef;
	}

	subpass.push_back(subpassDescr);
}

void VulkanRenderPass::AddSubpassDependency(DependencyTemplate depend_template, uint32_t srcSubpass, uint32_t dstSubpass)
{
	VkSubpassDependency depend = {};

	if (depend_template == DependencyTemplate::TEMPLATE_TOP_OF_PIPE) {

		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		depend.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		depend.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		depend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	else if (depend_template == DependencyTemplate::TEMPLATE_BOTTOM_OF_PIPE) {

		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		depend.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		depend.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	else if (depend_template == DependencyTemplate::TEMPLATE_MULTI_SUBPASS) {

		depend.srcSubpass = srcSubpass;
		depend.dstSubpass = dstSubpass;
		depend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		depend.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		depend.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	else if (depend_template == DependencyTemplate::TEMPLATE_DEPTH_STENCIL_SUBPASS_BOTTOM) {

		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		depend.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depend.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		depend.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	else if (depend_template == DependencyTemplate::TEMPLATE_DEPTH_STENCIL_SUBPASS_FRAG) {

		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		depend.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depend.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
	}
	dependency.push_back(depend);
}

void VulkanRenderPass::PrepareRenderPass(const VkDevice device)
{
	// if dependency container is empty, go with the default layout
	if (dependency.empty()) {

		VkSubpassDependency depend = {};
		depend.srcSubpass = VK_SUBPASS_EXTERNAL;
		depend.dstSubpass = 0;
		depend.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		depend.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		depend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency.push_back(depend);

		depend.srcSubpass = 0;
		depend.dstSubpass = VK_SUBPASS_EXTERNAL;
		depend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		depend.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		depend.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		depend.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		depend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		dependency.push_back(depend);
	}

	// if subpass vector empty, use default subpass layout
	if (subpass.empty()) {

		VkSubpassDescription sPassDescr = {};
		sPassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		if (!colorReference.empty()) {
			sPassDescr.colorAttachmentCount = static_cast<uint32_t>(colorReference.size());
			sPassDescr.pColorAttachments = colorReference.data();
		}
		if (!depthReference.empty()) {
			sPassDescr.pDepthStencilAttachment = depthReference.data();
		}
		subpass.push_back(sPassDescr);
	}

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attachment.size());
	createInfo.pAttachments = attachment.data();
	createInfo.subpassCount = static_cast<uint32_t>(subpass.size());
	createInfo.pSubpasses = subpass.data();
	createInfo.dependencyCount = static_cast<uint32_t>(dependency.size());
	createInfo.pDependencies = dependency.data();

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
	frameInfo.layers = layerCount;

	VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameInfo, nullptr, &frameBuffer))
}

void VulkanRenderPass::PrepareFramebuffer(std::vector<VkImageView>& imageView, uint32_t width, uint32_t height, const VkDevice device, uint32_t layerCount)
{
	assert(!imageView.empty());
	assert(renderpass != VK_NULL_HANDLE);

	VkFramebufferCreateInfo frameInfo = {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = renderpass;
	frameInfo.attachmentCount = static_cast<uint32_t>(imageView.size());
	frameInfo.pAttachments = imageView.data();
	frameInfo.width = width;
	frameInfo.height = height;
	frameInfo.layers = layerCount;

	VK_CHECK_RESULT(vkCreateFramebuffer(device, &frameInfo, nullptr, &frameBuffer))
}

void VulkanRenderPass::Destroy()
{
	// its not always the case that the user might create a framebuffer using this class, so check
	if (frameBuffer != VK_NULL_HANDLE) {
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
	}
	vkDestroyRenderPass(device, renderpass, nullptr);

	device = VK_NULL_HANDLE;
}

