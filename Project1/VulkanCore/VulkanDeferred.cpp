#include "VulkanDeferred.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/Vulkan_shadow.h"
#include "VulkanCore/VulkanPBR.h"
#include "VulkanCore/VulkanIBL.h"
#include "Systems/camera_system.h"
#include <gtc/matrix_transform.hpp>

VulkanDeferred::VulkanDeferred(VulkanEngine *engine, VulkanUtility *utility) :
	VulkanModule(utility),
	p_vkEngine(engine)
{
}

VulkanDeferred::~VulkanDeferred()
{
}

void VulkanDeferred::CreateRenderpassAttachmentInfo(VkImageLayout finalLayout, VkFormat format, const uint32_t attachCount, VkAttachmentDescription *attachDescr)
{
	attachDescr->format = format;
	attachDescr->samples = VK_SAMPLE_COUNT_1_BIT;								// used for MSAA 
	attachDescr->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDescr->storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// IMPORTANT: this needs to be set to store operations for this to work!!!
	attachDescr->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDescr->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDescr->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDescr->finalLayout = finalLayout;
}

void VulkanDeferred::CreateDeferredImage(VkFormat format, VkImageUsageFlagBits usageFlags, TextureInfo& imageInfo)
{	
	imageInfo.height = p_vkEngine->m_surface.extent.height;
	imageInfo.width = p_vkEngine->m_surface.extent.width;
	imageInfo.format = format;

	VkImageAspectFlags aspectFlags = 0;

	if (usageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}
	else if (usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	assert(aspectFlags > 0);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = imageInfo.width;
	image_info.extent.height = imageInfo.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usageFlags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &imageInfo.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, imageInfo.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &imageInfo.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, imageInfo.image, imageInfo.texture_mem, 0);

	// depth image view
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = imageInfo.image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->m_device.device, &createInfo, nullptr, &imageInfo.imageView));
}

void VulkanDeferred::PrepareDeferredFramebuffer()
{
	// initialise the colour buffer attachments for PBR
	CreateDeferredImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.position.imageInfo);		// positions
	CreateDeferredImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.normal.imageInfo);		// normals
	CreateDeferredImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.albedo.imageInfo);		// albedo colour buffer
	CreateDeferredImage(VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.ao.imageInfo);			// ao colour buffer
	CreateDeferredImage(VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.metallic.imageInfo);		// metallic colour buffer
	CreateDeferredImage(VK_FORMAT_R16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.roughness.imageInfo);	// roughness colour buffer

	// initialise the G buffer
	// required depth image format in order of preference
	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormat depthFormat = vkUtility->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	CreateDeferredImage(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_deferredInfo.depth.imageInfo);

	// prepare deferred render pass
	PrepareDeferredRenderpass();

	// frame buffers for eachg swap chain
	std::array<VkImageView, 8> attachments = {};
	m_deferredInfo.frameBuffers.resize(p_vkEngine->m_swapchain.imageCount);
	
	for (uint32_t c = 0; c < m_deferredInfo.frameBuffers.size(); ++c) {

		attachments[0] = p_vkEngine->m_imageView.images[c];
		attachments[1] = m_deferredInfo.position.imageInfo.imageView;
		attachments[2] = m_deferredInfo.normal.imageInfo.imageView;
		attachments[3] = m_deferredInfo.albedo.imageInfo.imageView;
		attachments[4] = m_deferredInfo.ao.imageInfo.imageView;
		attachments[5] = m_deferredInfo.metallic.imageInfo.imageView;
		attachments[6] = m_deferredInfo.roughness.imageInfo.imageView;
		attachments[7] = m_deferredInfo.depth.imageInfo.imageView;

		VkFramebufferCreateInfo frameInfo = {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = m_deferredInfo.renderPass;
		frameInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameInfo.pAttachments = attachments.data();
		frameInfo.width = p_vkEngine->m_surface.extent.width;
		frameInfo.height = p_vkEngine->m_surface.extent.height;
		frameInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->m_device.device, &frameInfo, nullptr, &m_deferredInfo.frameBuffers[c]));
	}
}

void VulkanDeferred::PrepareDeferredRenderpass()
{
	// Create attachment info for colour attachment, G buffer 
	std::array<VkAttachmentDescription, 8> attachDescr = {};
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, p_vkEngine->m_surface.format.format, 0, &attachDescr[0]);				// color attachment - for swap chain presentation
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.position.imageInfo.format, 1, &attachDescr[1]);			// position
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.normal.imageInfo.format, 2, &attachDescr[2]);			// normal
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.albedo.imageInfo.format, 3, &attachDescr[3]);			//	albedo
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.ao.imageInfo.format, 4, &attachDescr[4]);				//	ao
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.metallic.imageInfo.format, 5, &attachDescr[5]);			//	metallic
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.roughness.imageInfo.format, 6, &attachDescr[6]);		//	roughness
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_deferredInfo.depth.imageInfo.format, 7, &attachDescr[7]);	// depth

	std::array<VkSubpassDescription, 3> subpassDescr = {};

	// ================================ subpass one - fill G-buffers
	std::array<VkAttachmentReference, 7> colorRef1 = {};
	colorRef1[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// colour - swapchain present
	colorRef1[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// position
	colorRef1[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// normal
	colorRef1[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		//albedo
	colorRef1[4] = { 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// ao
	colorRef1[5] = { 5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// metallic
	colorRef1[6] = { 6, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// roughness

	VkAttachmentReference depthRef = { 7, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	subpassDescr[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescr[0].colorAttachmentCount = static_cast<uint32_t>(colorRef1.size());
	subpassDescr[0].pColorAttachments = colorRef1.data();
	subpassDescr[0].pDepthStencilAttachment = &depthRef;

	// =============================== subpass two - draw scene 
	std::array<VkAttachmentReference, 1> colorRef2 = {};
	colorRef2[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// colour - swapchain present

	std::array<VkAttachmentReference, 6> inputRef = {};
	inputRef[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// position
	inputRef[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// normal
	inputRef[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// albedo
	inputRef[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// ao
	inputRef[4] = { 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// metallic
	inputRef[5] = { 6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// roughness

	subpassDescr[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescr[1].colorAttachmentCount = static_cast<uint32_t>(colorRef2.size());
	subpassDescr[1].pColorAttachments = colorRef2.data();
	subpassDescr[1].pDepthStencilAttachment = &depthRef;
	subpassDescr[1].inputAttachmentCount = static_cast<uint32_t>(inputRef.size());
	subpassDescr[1].pInputAttachments = inputRef.data();

	// =============================== subpass three - draw skybox 
	std::array<VkAttachmentReference, 1> colorRef3 = {};
	colorRef3[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// colour - swapchain present

	subpassDescr[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescr[2].colorAttachmentCount = static_cast<uint32_t>(colorRef3.size());
	subpassDescr[2].pColorAttachments = colorRef3.data();
	subpassDescr[2].pDepthStencilAttachment = &depthRef;

	std::array<VkSubpassDependency, 4> sPassDepend = {};
	sPassDepend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[0].dstSubpass = 0;
	sPassDepend[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	sPassDepend[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[1].srcSubpass = 0;
	sPassDepend[1].dstSubpass = 1;
	sPassDepend[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	sPassDepend[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	sPassDepend[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[2].srcSubpass = 1;
	sPassDepend[2].dstSubpass = 2;
	sPassDepend[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	sPassDepend[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	sPassDepend[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[3].srcSubpass = 0;
	sPassDepend[3].dstSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	sPassDepend[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attachDescr.size());
	createInfo.pAttachments = attachDescr.data();
	createInfo.subpassCount = static_cast<uint32_t>(subpassDescr.size());
	createInfo.pSubpasses = subpassDescr.data();
	createInfo.dependencyCount = static_cast<uint32_t>(sPassDepend.size());
	createInfo.pDependencies = sPassDepend.data();

	VK_CHECK_RESULT(vkCreateRenderPass(p_vkEngine->m_device.device, &createInfo, nullptr, &m_deferredInfo.renderPass));
}

void VulkanDeferred::PrepareDeferredDescriptorSet()
{
	std::array<VkDescriptorPoolSize, 3> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 2;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descrPoolSize[1].descriptorCount = 6;
	descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	descrPoolSize[2].descriptorCount = 6;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_deferredInfo.descriptor.pool));

	// deferred descriptor layout
	std::array<VkDescriptorSetLayoutBinding, 12> layoutBind = {};
	layoutBind[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	layoutBind[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	layoutBind[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// position
	layoutBind[3] = vkUtility->InitLayoutBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// normal
	layoutBind[4] = vkUtility->InitLayoutBinding(4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// albedo
	layoutBind[5] = vkUtility->InitLayoutBinding(5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// ao
	layoutBind[6] = vkUtility->InitLayoutBinding(6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// metallic
	layoutBind[7] = vkUtility->InitLayoutBinding(7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// roughness
	layoutBind[8] = vkUtility->InitLayoutBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// shadow
	layoutBind[9] = vkUtility->InitLayoutBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// BDRF lut
	layoutBind[10] = vkUtility->InitLayoutBinding(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// irradiance map
	layoutBind[11] = vkUtility->InitLayoutBinding(11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// pre-filter map

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	layoutInfo.pBindings = layoutBind.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_deferredInfo.descriptor.layout));

	// Create descriptor set for meshes
	VkDescriptorSetLayout layouts[] = { m_deferredInfo.descriptor.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_deferredInfo.descriptor.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_deferredInfo.descriptor.set));

	std::array<VkDescriptorBufferInfo, 2> uboBufferInfo = {};
	uboBufferInfo[0] = vkUtility->InitBufferInfoDescriptor(m_buffers.vertexUbo.buffer, 0, m_buffers.vertexUbo.size);
	uboBufferInfo[1] = vkUtility->InitBufferInfoDescriptor(m_buffers.fragmentUbo.buffer, 0, m_buffers.fragmentUbo.size);

	auto vkShadow = p_vkEngine->VkModule<VulkanShadow>(VkModId::VKMOD_SHADOW_ID);
	auto vkPBR = p_vkEngine->VkModule<VulkanPBR>(VkModId::VKMOD_PBR_ID);
	auto vkIBL = p_vkEngine->VkModule<VulkanIBL>(VkModId::VKMOD_IBL_ID);

	std::array<VkDescriptorImageInfo, 10> imageInfo = {};
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.position.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[1] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.normal.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[2] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.albedo.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[3] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.ao.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[4] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.metallic.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[5] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.roughness.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[6] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, vkShadow->m_depthImage.imageView, vkShadow->m_depthImage.m_tex_sampler);
	imageInfo[7] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkPBR->lutImage.imageView, vkPBR->lutImage.m_tex_sampler);
	imageInfo[8] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkIBL->m_irradianceCube.cubeImage.imageView, vkIBL->m_irradianceCube.cubeImage.m_tex_sampler);
	imageInfo[9] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkIBL->m_filterCube.cubeImage.imageView, vkIBL->m_filterCube.cubeImage.m_tex_sampler);
	
	std::array<VkWriteDescriptorSet, 12> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBufferInfo[1]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[0]);
	writeDescrSet[3] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[1]);
	writeDescrSet[4] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[2]);
	writeDescrSet[5] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[3]);
	writeDescrSet[6] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[4]);
	writeDescrSet[7] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[5]);
	writeDescrSet[8] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[6]);
	writeDescrSet[9] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[7]);
	writeDescrSet[10] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[8]);
	writeDescrSet[11] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[9]);
	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanDeferred::PrepareDeferredPipeline()
{
	// offscreen pipeline
	Vertex vertex;
	auto bindingDescr = vertex.GetInputBindingDescription();
	auto attrDescr = vertex.GetAttrBindingDescription();

	VkPipelineVertexInputStateCreateInfo vertexInfo = {};
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexBindingDescriptions = &bindingDescr;
	vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescr.size());
	vertexInfo.pVertexAttributeDescriptions = attrDescr.data();

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {};
	assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->m_viewport.viewPort, p_vkEngine->m_viewport.scissor, 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility->InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	// colour attachment required for each colour buffer
	std::array<VkPipelineColorBlendAttachmentState, 1> colorAttach = {};
	colorAttach[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[0].blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorInfo = {};
	colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorInfo.logicOpEnable = VK_FALSE;
	colorInfo.attachmentCount = static_cast<uint32_t>(colorAttach.size());
	colorInfo.pAttachments = colorAttach.data();

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 3;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_deferredInfo.descriptor.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_deferredInfo.pipelineInfo.layout));

	// sahders for rendering full screen quad
	m_deferredInfo.shader[0] = vkUtility->InitShaders("deferred/deferred-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_deferredInfo.shader[1] = vkUtility->InitShaders("deferred/deferred-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_deferredInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_deferredInfo.pipelineInfo.layout;
	createInfo.renderPass = m_deferredInfo.renderPass;
	createInfo.subpass = 1;						// complete scene draw pass
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_deferredInfo.pipelineInfo.pipeline));
}

void VulkanDeferred::GenerateDeferredCmdBuffer(VkCommandBuffer cmdBuffer)
{
	VkDeviceSize offsets[1]{ 0 };

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredInfo.pipelineInfo.pipeline);
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_buffers.vertices.buffer, offsets);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredInfo.pipelineInfo.layout, 0, 1, &m_deferredInfo.descriptor.set, 0, NULL);

	vkCmdBindIndexBuffer(cmdBuffer, m_buffers.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
}

void VulkanDeferred::CreateUBOBuffers()
{
	// vertex UBO buffer
	m_buffers.vertexUbo.size = sizeof(VertexUBOLayout);
	vkUtility->CreateBuffer(m_buffers.vertexUbo.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffers.vertexUbo.buffer, m_buffers.vertexUbo.memory);

	// fragment UBO buffer
	m_buffers.fragmentUbo.size = sizeof(FragmentUBOLayout);
	vkUtility->CreateBuffer(m_buffers.fragmentUbo.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffers.fragmentUbo.buffer, m_buffers.fragmentUbo.memory);

	PreapareLightData();
}

void VulkanDeferred::PrepareFullscreenQuad()
{
	// prepare vertices
	std::vector<Vertex> vertices;
	vertices.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });
	vertices.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });
	vertices.push_back({ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });
	vertices.push_back({ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });

	// prepare indices
	std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
	for (uint32_t i = 0; i < 3; ++i)
	{
		uint32_t values[6] = { 0,1,2, 2,3,0 };
		for (auto index : values)
		{
			indices.push_back(i * 4 + index);
		}
	}

	// map to device memory
	m_buffers.vertices.size = sizeof(Vertex) * vertices.size();
	m_buffers.indices.size = sizeof(uint32_t) * indices.size();

	// map vertices
	vkUtility->CreateBuffer(m_buffers.vertices.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffers.vertices.buffer, m_buffers.vertices.memory);
	vkUtility->MapBuffer<Vertex>(m_buffers.vertices, vertices);

	// map indices
	vkUtility->CreateBuffer(m_buffers.indices.size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_buffers.indices.buffer, m_buffers.indices.memory);
	vkUtility->MapBuffer<uint32_t>(m_buffers.indices, indices);
}

void VulkanDeferred::PreapareLightData()
{
	m_fragBuffer.lights[0] = LightInfo(glm::vec4(-14.0f, -0.5f, 10.0f, 1.0f), glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 0.5f, 0.5f, 0.0f));
	m_fragBuffer.lights[1] = LightInfo(glm::vec4(-14.0f, -4.5f, 0.0f, 1.0f), glm::vec4(2.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
	m_fragBuffer.lights[2] = LightInfo(glm::vec4(-0.0f, -0.5f, 10.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[3] = LightInfo(glm::vec4(4.0f, -1.0f, 0.0f, 1.0f), glm::vec4(1.0f, 0.0f, 1.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[4] = LightInfo(glm::vec4(0.0f, -0.0f, -15.0f, 1.0f), glm::vec4(-0.5f, 1.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[5] = LightInfo(glm::vec4(-5.0f, -1.5f, 0.0f, 1.0f), glm::vec4(0.0f, -2.0f, 0.5f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[6] = LightInfo(glm::vec4(-2.0f, 15.0f, -5.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[7] = LightInfo(glm::vec4(-40.0f, -10.0f, -20.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[8] = LightInfo(glm::vec4(10.0f, -2.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[9] = LightInfo(glm::vec4(-1.0f, -3.0f, 1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
	m_fragBuffer.lights[10] = LightInfo(glm::vec4(-12.0f, -5.0f, 1.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
}

void VulkanDeferred::Init()
{
	PrepareDeferredFramebuffer();
	CreateUBOBuffers();
	PrepareFullscreenQuad();
	PrepareDeferredDescriptorSet();
	PrepareDeferredPipeline();
}

void VulkanDeferred::Update(CameraSystem *camera)
{
	auto vkShadow = p_vkEngine->VkModule<VulkanShadow>(VkModId::VKMOD_SHADOW_ID);

	// update vrtex ubo buffer
	m_vertBuffer.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
	m_vertBuffer.viewMatrix = glm::mat4(1.0f);
	m_vertBuffer.modelMatrix = glm::mat4(1.0f);

	vkUtility->MapBuffer<VertexUBOLayout>(m_buffers.vertexUbo, m_vertBuffer);

	// update fragment ubo buffer
	m_fragBuffer.viewPos = glm::vec4(camera->GetCameraPosition(), 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
	m_fragBuffer.cameraPos = glm::vec4(camera->GetCameraPosition(), 0.0f) * -1.0f;

	for (uint32_t c = 0; c < LIGHT_COUNT; ++c) {

		m_fragBuffer.lights[c].viewMatrix = vkShadow->m_shadowInfo.uboData.mvp[c];
	}

	vkUtility->MapBuffer<FragmentUBOLayout>(m_buffers.fragmentUbo, m_fragBuffer);
}

void VulkanDeferred::Destroy()
{

}

// shader setup
VkVertexInputBindingDescription VulkanDeferred::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 4> VulkanDeferred::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 4> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(Vertex, uv);

	// Vertex layout 2: normal
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(Vertex, normal);

	// Vertex layout 3: colour
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(Vertex, colour);

	return attrDescr;
}
