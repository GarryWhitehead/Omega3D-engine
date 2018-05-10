#include "VulkanIBL.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/VulkanModel.h"
#include "Systems/camera_system.h"

VulkanIBL::VulkanIBL(VulkanEngine *engine, VulkanUtility *utility) :
	VulkanModule(utility),
	p_vkEngine(engine)
{
}


VulkanIBL::~VulkanIBL()
{
}

void VulkanIBL::LoadAssets()
{
	m_cubeImage = vkUtility->LoadCubeMap("assets/textures/skybox/hdr_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, p_vkEngine->m_cmdPool);
}

void VulkanIBL::PrepareImage(VkFormat format, TextureInfo& imageInfo, int dim, int mipLevels)
{
	imageInfo.width = dim;
	imageInfo.height = dim;
	imageInfo.mipLevels = mipLevels;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = imageInfo.width;
	image_info.extent.height = imageInfo.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = imageInfo.mipLevels;
	image_info.arrayLayers = 6;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

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
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 6;
	createInfo.subresourceRange.levelCount = imageInfo.mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->m_device.device, &createInfo, nullptr, &imageInfo.imageView));

	vkUtility->CreateTextureSampler(imageInfo, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1, VK_COMPARE_OP_NEVER, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
}

void VulkanIBL::PrepareRenderpass(VkFormat format, VkRenderPass& renderpass)
{
	VkAttachmentDescription colorAttach = {};
	colorAttach.format = format;
	colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttach.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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
	sPassDescr.colorAttachmentCount = 1;
	sPassDescr.pColorAttachments = &colorRef;

	std::vector<VkAttachmentDescription> attach = { colorAttach };
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attach.size());
	createInfo.pAttachments = attach.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sPassDescr;
	createInfo.dependencyCount = static_cast<uint32_t>(sPassDepend.size());
	createInfo.pDependencies = sPassDepend.data();

	VK_CHECK_RESULT(vkCreateRenderPass(p_vkEngine->m_device.device, &createInfo, nullptr, &renderpass));
}

void VulkanIBL::PrepareOffscreenFrameBuffer(VkFormat format, TextureInfo& imageInfo, VkRenderPass renderpass, int dim, VkFramebuffer& framebuffer)
{
	imageInfo.width = dim;
	imageInfo.height = dim;
	
	// the irradiance map will be rendered into an offscreen buffer
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
	image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &imageInfo.image));

	vkUtility->ImageTransition(VK_NULL_HANDLE, imageInfo.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->m_cmdPool);

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, imageInfo.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &imageInfo.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, imageInfo.image, imageInfo.texture_mem, 0);

	// create ofscreen image view
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = imageInfo.image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->m_device.device, &createInfo, nullptr, &imageInfo.imageView));

	// create offscreen frame buffer
	VkFramebufferCreateInfo frameInfo = {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = renderpass;
	frameInfo.attachmentCount = 1;
	frameInfo.pAttachments = &imageInfo.imageView;
	frameInfo.width = imageInfo.width;
	frameInfo.height = imageInfo.height;
	frameInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->m_device.device, &frameInfo, nullptr, &framebuffer))
}

void VulkanIBL::PrepareIBLDescriptors()
{
	std::array<VkDescriptorPoolSize, 1> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descrPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_descriptors.pool));

	std::array<VkDescriptorSetLayoutBinding, 1> layoutBind;
	layoutBind[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// bindings for the colour image sampler

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	layoutInfo.pBindings = layoutBind.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_descriptors.layout));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_descriptors.set));

	std::array<VkDescriptorImageInfo, 1> imageInfo = {};
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_cubeImage.imageView, m_cubeImage.m_tex_sampler);		

	std::array<VkWriteDescriptorSet, 1> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_descriptors.set, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[0]);

	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanIBL::PrepareIBLPipeline()
{
	// offscreen pipeline
	VulkanModel::ModelVertex vertex;
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

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

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

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 2;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_FALSE;
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(FilterPushConstant);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_descriptors.layout;
	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_pipelineLayout));

	// sahders for rendering full screen quad
	m_shader[0] = vkUtility->InitShaders("IBL/cubemap-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("IBL/cubemap_irradiance-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_pipelineLayout;
	createInfo.renderPass = m_irradianceCube.renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_irradianceCube.pipeline));

	// pre-filter pipeline
	createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;											
	createInfo.basePipelineHandle = m_irradianceCube.pipeline;
	createInfo.renderPass = m_filterCube.renderpass;
	m_shader[1] = vkUtility->InitShaders("IBL/cubemap_prefilter-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_filterCube.pipeline));
}

void VulkanIBL::GenerateIrrMapCmdBuffer()
{
	std::array<VkClearValue, 1> clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_irradianceCube.renderpass;
	renderPassInfo.framebuffer = m_irradianceCube.offscreenFB;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = m_irradianceCube.cubeImage.width;
	renderPassInfo.renderArea.extent.height = m_irradianceCube.cubeImage.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();
	
	VkCommandBuffer cmdBuffer = vkUtility->CreateCmdBuffer(vkUtility->VK_PRIMARY, vkUtility->VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->m_cmdPool);

	VkViewport viewport = vkUtility->InitViewPort(m_irradianceCube.cubeImage.width, m_irradianceCube.cubeImage.height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(m_irradianceCube.cubeImage.width, m_irradianceCube.cubeImage.height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// the cube map object data is loaded with all other models - it is assumed that the cubemap will default at index location 0 in the model map - TODO: ref cubemap data location in .json world data when implemented
	auto p_vkModel = p_vkEngine->VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID);
	VulkanModel::ModelInfo model = p_vkModel->RequestModelInfo(0);

	VkDeviceSize offsets[1]{ 0 };

	vkUtility->ImageTransition(cmdBuffer, m_irradianceCube.cubeImage.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_vkEngine->m_cmdPool, MIP_LEVELS, 6);

	for (int m = 0; m < MIP_LEVELS; ++m) {
		
		for (uint32_t c = 0; c < 6; ++c) {

			float dim = static_cast<float>(IRRADIANCEMAP_DIM * std::pow(0.5f, m));
			VkViewport viewport = vkUtility->InitViewPort(dim, dim, 0.0f, 1.0f);

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_irradianceCube.pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptors.set, 0, NULL);
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkModel->GetVertexBuffer(), offsets);
			vkCmdBindIndexBuffer(cmdBuffer, p_vkModel->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// render each of the cubes faces
			FilterPushConstant push;
			push.mvp = glm::perspective(PI / 2, 1.0f, 0.1f, 512.0f) * cubeView[c];
			vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FilterPushConstant), &push);

			vkCmdDrawIndexed(cmdBuffer, model.meshes[0].indexCount, 1, 0, 0, 0);
			vkCmdEndRenderPass(cmdBuffer);

			// copy each of the cubemap faces 
			vkUtility->ImageTransition(cmdBuffer, m_irradianceCube.offscreenImage.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_vkEngine->m_cmdPool);

			VkImageCopy imageCopy = {};
			imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.srcSubresource.baseArrayLayer = 0;
			imageCopy.srcSubresource.layerCount = 1;
			imageCopy.srcSubresource.mipLevel = 0;
			imageCopy.srcOffset = { 0,0,0 };

			imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.dstSubresource.layerCount = 1;
			imageCopy.dstSubresource.mipLevel = m;
			imageCopy.dstSubresource.baseArrayLayer = c;
			imageCopy.dstOffset = { 0,0,0 };
			imageCopy.extent.depth = 1;

			vkCmdCopyImage(cmdBuffer, m_irradianceCube.offscreenImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_irradianceCube.cubeImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

			vkUtility->ImageTransition(cmdBuffer, m_irradianceCube.offscreenImage.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->m_cmdPool);
		}
	}
	vkUtility->ImageTransition(cmdBuffer, m_irradianceCube.cubeImage.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, p_vkEngine->m_cmdPool, MIP_LEVELS, 6);

	vkUtility->SubmitCmdBufferToQueue(cmdBuffer, p_vkEngine->m_queue.graphQueue);
}

void VulkanIBL::GeneratePreFilterCmdBuffer()
{
	std::array<VkClearValue, 1> clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_filterCube.renderpass;
	renderPassInfo.framebuffer = m_filterCube.offscreenFB;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = m_filterCube.cubeImage.width;
	renderPassInfo.renderArea.extent.height = m_filterCube.cubeImage.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();

	VkCommandBuffer cmdBuffer = vkUtility->CreateCmdBuffer(vkUtility->VK_PRIMARY, vkUtility->VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->m_cmdPool);

	VkViewport viewport = vkUtility->InitViewPort(m_filterCube.cubeImage.width, m_filterCube.cubeImage.height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(m_filterCube.cubeImage.width, m_filterCube.cubeImage.height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// the cube map object data is loaded with all other models - it is assumed that the cubemap will default at index location 0 in the model map - TODO: ref cubemap data location in .json world data when implemented
	auto p_vkModel = p_vkEngine->VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID);
	VulkanModel::ModelInfo model = p_vkModel->RequestModelInfo(0);

	VkDeviceSize offsets[1]{ 0 };

	vkUtility->ImageTransition(cmdBuffer, m_filterCube.cubeImage.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_vkEngine->m_cmdPool, MIP_LEVELS, 6);

	FilterPushConstant push;
	for (int m = 0; m < MIP_LEVELS; ++m) {

		push.roughness = (float)m / (float)(MIP_LEVELS - 1);

		for (uint32_t c = 0; c < 6; ++c) {

			float dim = static_cast<float>(PREFILTERMAP_DIM * std::pow(0.5f, m));
			VkViewport viewport = vkUtility->InitViewPort(dim, dim, 0.0f, 1.0f);

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_filterCube.pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptors.set, 0, NULL);
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkModel->GetVertexBuffer(), offsets);
			vkCmdBindIndexBuffer(cmdBuffer, p_vkModel->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// render each of the cubes faces
			push.mvp = glm::perspective(PI / 2, 1.0f, 0.1f, 512.0f) * cubeView[c];
			push.sampleCount = 1024;
			vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FilterPushConstant), &push);

			vkCmdDrawIndexed(cmdBuffer, model.meshes[0].indexCount, 1, 0, 0, 0);
			vkCmdEndRenderPass(cmdBuffer);

			// copy each of the cubemap faces 
			vkUtility->ImageTransition(cmdBuffer, m_filterCube.offscreenImage.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_vkEngine->m_cmdPool);

			VkImageCopy imageCopy = {};
			imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.srcSubresource.baseArrayLayer = 0;
			imageCopy.srcSubresource.layerCount = 1;
			imageCopy.srcSubresource.mipLevel = 0;
			imageCopy.srcOffset = { 0,0,0 };

			imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.dstSubresource.layerCount = 1;
			imageCopy.dstSubresource.mipLevel = m;
			imageCopy.dstSubresource.baseArrayLayer = c;
			imageCopy.dstOffset = { 0,0,0 };
			imageCopy.extent.depth = 1;

			vkCmdCopyImage(cmdBuffer, m_filterCube.offscreenImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_filterCube.cubeImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

			vkUtility->ImageTransition(cmdBuffer, m_filterCube.offscreenImage.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->m_cmdPool);		
		}
	}
	vkUtility->ImageTransition(cmdBuffer, m_filterCube.cubeImage.image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, p_vkEngine->m_cmdPool, MIP_LEVELS, 6);

	vkUtility->SubmitCmdBufferToQueue(cmdBuffer, p_vkEngine->m_queue.graphQueue);
}

void VulkanIBL::Update(CameraSystem *camera)
{	
}

void VulkanIBL::Init()
{
	// load the environment cube
	LoadAssets();

	// generate the image, FB and renderpass for the irradiance calculations
	VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	PrepareImage(format, m_irradianceCube.cubeImage, IRRADIANCEMAP_DIM, MIP_LEVELS);
	PrepareRenderpass(format, m_irradianceCube.renderpass);
	PrepareOffscreenFrameBuffer(format, m_irradianceCube.offscreenImage, m_irradianceCube.renderpass, IRRADIANCEMAP_DIM, m_irradianceCube.offscreenFB);

	// generate the image, FB and renderpass for the pre-filtered cube
	format = VK_FORMAT_R16G16B16A16_SFLOAT;
	PrepareImage(format, m_filterCube.cubeImage, PREFILTERMAP_DIM, MIP_LEVELS);
	PrepareRenderpass(format, m_filterCube.renderpass);
	PrepareOffscreenFrameBuffer(format, m_filterCube.offscreenImage, m_filterCube.renderpass, PREFILTERMAP_DIM, m_filterCube.offscreenFB);

	// prepare descriptors and pipelines for both
	PrepareIBLDescriptors();
	PrepareIBLPipeline();
}

void VulkanIBL::Destroy()
{

}