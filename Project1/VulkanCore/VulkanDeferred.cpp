#include "VulkanDeferred.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/Vulkan_shadow.h"
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

void VulkanDeferred::CreateRenderpassAttachmentInfo(VkImageLayout finalLayout, VkFormat format, const uint32_t attachCount, VkAttachmentDescription *attachDescr, VkAttachmentReference *attachRef)
{
	attachDescr->format = format;
	attachDescr->samples = SAMPLE_COUNT;								// used for MSAA 
	attachDescr->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDescr->storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// IMPORTANT: this needs to be set to store operations for this to work!!!
	attachDescr->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDescr->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDescr->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDescr->finalLayout = finalLayout;

	if (attachRef != VK_NULL_HANDLE) {
		attachRef->attachment = attachCount;
		attachRef->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
}

void VulkanDeferred::CreateDeferredImage(VkFormat format, VkImageUsageFlagBits usageFlags, TextureInfo& imageInfo)
{	
	imageInfo.height = DEFERRED_SIZE;
	imageInfo.width = DEFERRED_SIZE;
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
	image_info.usage = usageFlags | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = SAMPLE_COUNT;

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

	vkUtility->CreateTextureSampler(imageInfo, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1, VK_COMPARE_OP_NEVER, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
}

void VulkanDeferred::PrepareDeferredFramebuffer()
{
	// initialise the colour buffer attachments
	CreateDeferredImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.position.imageInfo);		// positions
	CreateDeferredImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.normal.imageInfo);		// normals
	CreateDeferredImage(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_deferredInfo.albedo.imageInfo);			// albedo colour buffer
	
	// initialise the G buffer
	CreateDeferredImage(p_vkEngine->m_depthImageFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_deferredInfo.depth.imageInfo);

	// prepare deferred render pass
	PrepareDeferredRenderpass();

	// offscreen frame buffer
	std::vector<VkImageView> colourAttachments{
		m_deferredInfo.position.imageInfo.imageView,
		m_deferredInfo.normal.imageInfo.imageView,
		m_deferredInfo.albedo.imageInfo.imageView,
		m_deferredInfo.depth.imageInfo.imageView
	};

	VkFramebufferCreateInfo frameInfo = {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = m_deferredInfo.renderPass;
	frameInfo.attachmentCount = static_cast<uint32_t>(colourAttachments.size());
	frameInfo.pAttachments = colourAttachments.data();
	frameInfo.width = DEFERRED_SIZE;
	frameInfo.height = DEFERRED_SIZE;
	frameInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->m_device.device, &frameInfo, nullptr, &m_deferredInfo.frameBuffer))
}

void VulkanDeferred::PrepareDeferredRenderpass()
{
	// Create attachment info for colour and G buffer
	std::array<VkAttachmentDescription, 4> attachDescr = {};
	std::array<VkAttachmentReference, 3> attachRef = {};

	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.position.imageInfo.format, 0, &attachDescr[0], &attachRef[0]);	// position
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.normal.imageInfo.format, 1, &attachDescr[1], &attachRef[1]);		// normal
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.albedo.imageInfo.format, 2, &attachDescr[2], &attachRef[2]);		//	albedo
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_deferredInfo.depth.imageInfo.format, 3, &attachDescr[3], VK_NULL_HANDLE);			// depth

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 3;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

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
	sPassDescr.pColorAttachments = attachRef.data();
	sPassDescr.colorAttachmentCount = static_cast<uint32_t>(attachRef.size());
	sPassDescr.pDepthStencilAttachment = &depthRef;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attachDescr.size());
	createInfo.pAttachments = attachDescr.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sPassDescr;
	createInfo.dependencyCount = static_cast<uint32_t>(sPassDepend.size());
	createInfo.pDependencies = sPassDepend.data();

	VK_CHECK_RESULT(vkCreateRenderPass(p_vkEngine->m_device.device, &createInfo, nullptr, &m_deferredInfo.renderPass));
}

void VulkanDeferred::PrepareDeferredDescriptorSet()
{
	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 2;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descrPoolSize[1].descriptorCount = 5;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_deferredInfo.descriptor.pool));

	// deferred descriptor layout
	std::array<VkDescriptorSetLayoutBinding, 6> layoutBind = {};
	layoutBind[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	layoutBind[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	layoutBind[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// position
	layoutBind[3] = vkUtility->InitLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// normal
	layoutBind[4] = vkUtility->InitLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// albedo
	layoutBind[5] = vkUtility->InitLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// shadow

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
	std::array<VkDescriptorImageInfo, 4> imageInfo = {};
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.position.imageInfo.imageView, m_deferredInfo.position.imageInfo.m_tex_sampler);
	imageInfo[1] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.normal.imageInfo.imageView, m_deferredInfo.normal.imageInfo.m_tex_sampler);
	imageInfo[2] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.albedo.imageInfo.imageView, m_deferredInfo.albedo.imageInfo.m_tex_sampler);
	imageInfo[3] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, vkShadow->m_depthImage.imageView, vkShadow->m_depthImage.m_tex_sampler);

	std::array<VkWriteDescriptorSet, 6> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBufferInfo[1]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[0]);
	writeDescrSet[3] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[1]);
	writeDescrSet[4] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[2]);
	writeDescrSet[5] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[3]);
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

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);

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
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_deferredInfo.descriptor.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_deferredInfo.pipelineInfo.layout));

	// specialisation constant to specify sampling count at the shader
	uint32_t specData = SAMPLE_COUNT;

	VkSpecializationMapEntry mapEntry;
	mapEntry.constantID = 0;
	mapEntry.offset = 0;
	mapEntry.size = sizeof(uint32_t);

	VkSpecializationInfo specialInfo;
	specialInfo.mapEntryCount = 1;
	specialInfo.pMapEntries = &mapEntry;
	specialInfo.pData = &specData;
	specialInfo.dataSize = sizeof(specData);

	// sahders for rendering full screen quad
	m_deferredInfo.shader[0] = vkUtility->InitShaders("deferred/deferred-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_deferredInfo.shader[1] = vkUtility->InitShaders("deferred/deferred-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_deferredInfo.shader[1].pSpecializationInfo = &specialInfo;
	
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
	createInfo.renderPass = p_vkEngine->m_renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_deferredInfo.pipelineInfo.pipeline));
}

void VulkanDeferred::GenerateDeferredCmdBuffer(VkCommandBuffer cmdBuffer)
{
	std::array<VkClearValue, 4> clearValues = {};
	clearValues[0].color = p_vkEngine->CLEAR_COLOR;
	clearValues[1].color = p_vkEngine->CLEAR_COLOR;
	clearValues[2].color = p_vkEngine->CLEAR_COLOR;
	clearValues[3].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_deferredInfo.renderPass;
	renderPassInfo.framebuffer = m_deferredInfo.frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = DEFERRED_SIZE;
	renderPassInfo.renderArea.extent.height = DEFERRED_SIZE;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkViewport viewport = vkUtility->InitViewPort(DEFERRED_SIZE, DEFERRED_SIZE, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(DEFERRED_SIZE, DEFERRED_SIZE, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// render the scene into the offscreen frame bnuffer
	p_vkEngine->RenderScene(cmdBuffer, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE);		// draw into dferred buffers using modules own descriptors

	vkCmdEndRenderPass(cmdBuffer);
}

void VulkanDeferred::GenerateFullscreenCmdBuffers()
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = p_vkEngine->CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	m_cmdBuffers.resize(p_vkEngine->m_frameBuffer.size());

	for (uint32_t c = 0; c < m_cmdBuffers.size(); ++c) {

		m_cmdBuffers[c] = vkUtility->CreateCmdBuffer(vkUtility->VK_PRIMARY, vkUtility->VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->m_cmdPool);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = p_vkEngine->m_frameBuffer[c];
		renderPassInfo.renderPass = p_vkEngine->m_renderpass;
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent.width = p_vkEngine->m_surface.extent.width;
		renderPassInfo.renderArea.extent.height = p_vkEngine->m_surface.extent.height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VkViewport viewport = vkUtility->InitViewPort(p_vkEngine->m_surface.extent.width, p_vkEngine->m_surface.extent.height, 0.0f, 1.0f);
		vkCmdSetViewport(m_cmdBuffers[c], 0, 1, &viewport);

		VkRect2D scissor = vkUtility->InitScissor(p_vkEngine->m_surface.extent.width, p_vkEngine->m_surface.extent.height, 0, 0);
		vkCmdSetScissor(m_cmdBuffers[c], 0, 1, &scissor);

		vkCmdBeginRenderPass(m_cmdBuffers[c], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		// final scene rendered as a full screen quad
		VkDeviceSize offsets[1]{ 0 };

		vkCmdBindPipeline(m_cmdBuffers[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredInfo.pipelineInfo.pipeline);
		vkCmdBindVertexBuffers(m_cmdBuffers[c], 0, 1, &m_buffers.vertices.buffer, offsets);
		vkCmdBindDescriptorSets(m_cmdBuffers[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredInfo.pipelineInfo.layout, 0, 1, &m_deferredInfo.descriptor.set, 0, NULL);

		vkCmdBindIndexBuffer(m_cmdBuffers[c], m_buffers.indices.buffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(m_cmdBuffers[c], 6, 1, 0, 0, 0);

		// end of the command definitions for this render pass so tell the GPU
		vkCmdEndRenderPass(m_cmdBuffers[c]);
		VK_CHECK_RESULT(vkEndCommandBuffer(m_cmdBuffers[c]));
	}
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
	m_fragBuffer.lights[2] = LightInfo(glm::vec4(-14.0f, -0.5f, 10.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, 0.3f, 0.9f, 0.0f));
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
