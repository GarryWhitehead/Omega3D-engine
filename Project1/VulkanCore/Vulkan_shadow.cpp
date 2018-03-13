#include "Vulkan_shadow.h"
#include "VulkanCore/vulkan_scene.h"


VulkanShadow::VulkanShadow()
{
}

VulkanShadow::VulkanShadow(VulkanScene* vulkanScene)
{
	p_vulkanScene = vulkanScene;
}

VulkanShadow::~VulkanShadow()
{
}

void VulkanShadow::PrepareDepthImage()
{
	m_depthInfo.width = SHADOWMAP_WIDTH;
	m_depthInfo.height = SHADOWMAP_HEIGHT;
	
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = p_vulkanScene->m_depthImageFormat;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = m_depthInfo.width;
	image_info.extent.height = m_depthInfo.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = CSM_COUNT;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(p_vulkanScene->m_device.device, &image_info, nullptr, &m_depthInfo.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vulkanScene->m_device.device, m_depthInfo.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vulkanScene->m_device.device, &alloc_info, nullptr, &m_depthInfo.texture_mem));

	vkBindImageMemory(p_vulkanScene->m_device.device, m_depthInfo.image, m_depthInfo.texture_mem, 0);

	// depth view for all layers
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_depthInfo.image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	createInfo.format = p_vulkanScene->m_depthImageFormat;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = CSM_COUNT;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(p_vulkanScene->m_device.device, &createInfo, nullptr, &m_depthInfo.imageView));

	// create a depth buffer and frame buffer for each cascade
	for (uint32_t c = 0; c < CSM_COUNT; ++c) {
		
		// cascade image view
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = m_depthInfo.image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = p_vulkanScene->m_depthImageFormat;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = c;

		VK_CHECK_RESULT(vkCreateImageView(p_vulkanScene->m_device.device, &createInfo, nullptr, &m_cascadeInfo.imageView[c]));

		// frame buffer
		VkFramebufferCreateInfo frameInfo = {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = p_vulkanScene->m_renderpass;
		frameInfo.attachmentCount = 1;
		frameInfo.pAttachments = &m_cascadeInfo.imageView[c];
		frameInfo.width = m_depthInfo.width;
		frameInfo.height = m_depthInfo.height;
		frameInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(p_vulkanScene->m_device.device, &frameInfo, nullptr, &m_cascadeInfo.frameBuffer[c]));
	}

	// create generic texture sampler for all cascades
	CreateTextureSampler(m_depthInfo, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1, VK_COMPARE_OP_NEVER, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
}

void VulkanShadow::GenerateCascadeCmdBuffer()
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = p_vulkanScene->CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	m_cascadeInfo.cmdBuffer = CreateCmdBuffer(VK_PRIMARY, VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vulkanScene->m_cmdPool);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	
	renderPassInfo.renderPass = p_vulkanScene->m_renderpass;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = SHADOWMAP_WIDTH;
	renderPassInfo.renderArea.extent.height = SHADOWMAP_HEIGHT;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	// generate command buffer for each cascade
	for (uint32_t c = 0; c < CSM_COUNT; ++c) {
		
		renderPassInfo.framebuffer = m_cascadeInfo.frameBuffer[c];
		vkCmdBeginRenderPass(m_cascadeInfo.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		vkCmdBindPipeline(m_cascadeInfo.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_cascadeInfo.pipeline.pipeline);
		
		vkCmdEndRenderPass(m_cascadeInfo.cmdBuffer);
	}
	
	VK_CHECK_RESULT(vkEndCommandBuffer(m_cascadeInfo.cmdBuffer));
}