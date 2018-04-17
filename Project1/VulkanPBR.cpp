#include "VulkanPBR.h"
#include "VulkanCore/VulkanEngine.h"


VulkanPBR::VulkanPBR(VulkanEngine *engine) :
	p_vkEngine(engine)
{
}


VulkanPBR::~VulkanPBR()
{
}

void VulkanPBR::PrepareLUT()
{
	lutImage.width = LUT_DIM;
	lutImage.height = LUT_DIM;
	
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = VK_FORMAT_R16G16_SFLOAT;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = lutImage.width;
	image_info.extent.height = lutImage.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &lutImage.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, lutImage.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &lutImage.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, lutImage.image, lutImage.texture_mem, 0);

	// depth image view
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = lutImage.image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = VK_FORMAT_R16G16_SFLOAT;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->m_device.device, &createInfo, nullptr, &lutImage.imageView));

	vkUtility->CreateTextureSampler(lutImage, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1, VK_COMPARE_OP_NEVER, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
}

void VulkanPBR::PrepareLUTRenderpass()
{
	VkAttachmentDescription colorAttach = {};
	colorAttach.format = VK_FORMAT_R16G16_SFLOAT;
	colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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

	VK_CHECK_RESULT(vkCreateRenderPass(m_device.device, &createInfo, nullptr, &m_lutRenderpass));
}

void VulkanPBR::PrepareLUTFramebuffer()
{
	VkFramebufferCreateInfo frameInfo = {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = m_lutRenderpass;
	frameInfo.attachmentCount = 1;
	frameInfo.pAttachments = &lutImage.image;
	frameInfo.width = lutImage.width;
	frameInfo.height = lutImage.height;
	frameInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->m_device.device, &frameInfo, nullptr, &m_lutFramebuffer))
}

void VulkanPBR::PrepareLUTPipeline()
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

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 0;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_lutLayout));

	// sahders for rendering full screen quad
	m_lutShader[0] = vkUtility->InitShaders("BDRF/lutBDRF-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_lutShader[1] = vkUtility->InitShaders("BDRF/lutBDRF-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_lutShader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_lutLayout;
	createInfo.renderPass = m_lutRenderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_lutPipeline));
}

void VulkanPBR::GenerateLUTCmdBuffer()
{
	std::array<VkClearValue, 1> clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };
	
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_lutRenderpass;
	renderPassInfo.framebuffer = m_lutFramebuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = lutImage.width;
	renderPassInfo.renderArea.extent.height = lutImage.height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();

	m_lutCmdBuffer = vkUtility->CreateCmdBuffer(vkUtility->VK_PRIMARY, vkUtility->VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->m_cmdPool);

	VkViewport viewport = vkUtility->InitViewPort(lutImage.width, lutImage.height, 0.0f, 1.0f);
	vkCmdSetViewport(m_lutCmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(lutImage.width, lutImage.height, 0, 0);
	vkCmdSetScissor(m_lutCmdBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(m_lutCmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(m_lutCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lutPipeline);
	vkCmdDraw(m_lutCmdBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(m_lutCmdBuffer);
}

VkVertexInputBindingDescription VulkanPBR::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 1> VulkanPBR::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: uv
	std::array<VkVertexInputAttributeDescription, 1> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, uv);

	return attrDescr;
}