#include "Vulkan_shadow.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_model.h"
#include "VulkanCore/vulkan_terrain.h"
#include "Systems/camera_system.h"
#include <gtc/matrix_transform.hpp>
#include <algorithm>

VulkanShadow::VulkanShadow()
{
}

VulkanShadow::VulkanShadow(VulkanEngine* engine) :
	p_vkEngine(engine)
{
	vkUtility.InitVulkanUtility(engine);
}

VulkanShadow::~VulkanShadow()
{
}

void VulkanShadow::PrepareDepthBuffer()
{
	m_depthImage.width = SHADOWMAP_WIDTH;
	m_depthImage.height = SHADOWMAP_HEIGHT;
	
	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = p_vkEngine->m_depthImageFormat;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = m_depthImage.width;
	image_info.extent.height = m_depthImage.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &m_depthImage.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, m_depthImage.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = vkUtility.FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &m_depthImage.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, m_depthImage.image, m_depthImage.texture_mem, 0);

	// depth image view
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = m_depthImage.image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = p_vkEngine->m_depthImageFormat;
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->m_device.device, &createInfo, nullptr, &m_depthImage.imageView));

	// offscreen frame buffer
	VkFramebufferCreateInfo frameInfo = {};
	frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameInfo.renderPass = m_offscreenInfo.renderpass;
	frameInfo.attachmentCount = 1;
	frameInfo.pAttachments = &m_depthImage.imageView;
	frameInfo.width = m_depthImage.width;
	frameInfo.height = m_depthImage.height;
	frameInfo.layers = 1;

	VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->m_device.device, &frameInfo, nullptr, &m_offscreenInfo.frameBuffer))

	// create generic texture sampler for all cascades
	vkUtility.CreateTextureSampler(m_depthImage, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 1, VK_COMPARE_OP_NEVER, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);
}

void VulkanShadow::PrepareOffscreenDescriptors()
{
	// prepare descriptor sets for depth sampling - will be used in conjuction with other vulkan module sets
	const int BIND_COUNT = 1;
	VkDescriptorPoolSize descrPoolSize[1] = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = descrPoolSize;
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_offscreenInfo.descriptors.pool));

	VkDescriptorSetLayoutBinding uboLayoutVS = vkUtility.InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	
	VkDescriptorSetLayoutBinding descrDepthBind[] = { uboLayoutVS };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = BIND_COUNT;
	layoutInfo.pBindings = descrDepthBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_offscreenInfo.descriptors.layout));

	VkDescriptorSetLayout layouts[] = { m_offscreenInfo.descriptors.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_offscreenInfo.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_offscreenInfo.descriptors.set));
	
	// depth descriptor - vertex ubo and fragment shader
	VkDescriptorBufferInfo uboBuffInfoVS = vkUtility.InitBufferInfoDescriptor(m_offscreenInfo.uboBuffer.buffer, 0, m_offscreenInfo.uboBuffer.size);

	std::array<VkWriteDescriptorSet, 1> writeDescrSet = {};
	writeDescrSet[0] = vkUtility.InitDescriptorSet(m_offscreenInfo.descriptors.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfoVS);

	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanShadow::PrepareOffscreenRenderpass()
{
	VkAttachmentDescription depthAttach = {};
	depthAttach.format = p_vkEngine->m_depthImageFormat;
	depthAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 0;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	std::array<VkSubpassDependency, 2> sPassDepend = {};
	sPassDepend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[0].dstSubpass = 0;
	sPassDepend[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[0].dstStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	sPassDepend[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	sPassDepend[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	sPassDepend[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[1].srcSubpass = 0;
	sPassDepend[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	sPassDepend[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	sPassDepend[1].srcAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	sPassDepend[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	sPassDepend[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkSubpassDescription sPassDescr = {};
	sPassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sPassDescr.colorAttachmentCount = 0;
	sPassDescr.pDepthStencilAttachment = &depthRef;

	std::vector<VkAttachmentDescription> attach = { depthAttach };
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attach.size());
	createInfo.pAttachments = attach.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sPassDescr;
	createInfo.dependencyCount = static_cast<uint32_t>(sPassDepend.size());
	createInfo.pDependencies = sPassDepend.data();

	VK_CHECK_RESULT(vkCreateRenderPass(p_vkEngine->m_device.device, &createInfo, nullptr, &m_offscreenInfo.renderpass));
}

void VulkanShadow::PrepareOffscreenPipeline()
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

	VkPipelineViewportStateCreateInfo viewportState = vkUtility.InitViewPortCreateInfo(p_vkEngine->m_viewport.viewPort, p_vkEngine->m_viewport.scissor, 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility.InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE);
	rasterInfo.depthBiasEnable = VK_TRUE;

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility.InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	VkPipelineColorBlendStateCreateInfo colorInfo = {};
	colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorInfo.attachmentCount = 0;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = dynamicStates;
	dynamicInfo.dynamicStateCount = 1;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_offscreenInfo.descriptors.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_offscreenInfo.pipelineInfo.layout));

	// load the shaders with tyexture samplers for material textures
	m_offscreenInfo.shader[0] = vkUtility.InitShaders("Shadow/shadow-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_offscreenInfo.shader[1] = vkUtility.InitShaders("Shadow/shadow-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_offscreenInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_offscreenInfo.pipelineInfo.layout;
	createInfo.renderPass = m_offscreenInfo.renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_offscreenInfo.pipelineInfo.pipeline));
}

void VulkanShadow::GenerateOffscreenCmdBuffer()
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = p_vkEngine->CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	m_offscreenInfo.cmdBuffer = vkUtility.CreateCmdBuffer(vkUtility.VK_PRIMARY, vkUtility.VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->m_cmdPool);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	
	renderPassInfo.renderPass = m_offscreenInfo.renderpass;
	renderPassInfo.framebuffer = m_offscreenInfo.frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = SHADOWMAP_WIDTH;
	renderPassInfo.renderArea.extent.height = SHADOWMAP_HEIGHT;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	
	// to prevent mapping artefacts
	vkCmdSetDepthBias(m_offscreenInfo.cmdBuffer, biasConstant, 0.0f, biasSlope);

	VkViewport viewport = vkUtility.InitViewPort(SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0.0f, 1.0f);
	vkCmdSetViewport(m_offscreenInfo.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility.InitScissor(SHADOWMAP_WIDTH, SHADOWMAP_HEIGHT, 0, 0);
	vkCmdSetScissor(m_offscreenInfo.cmdBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(m_offscreenInfo.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// render the scene into the offscreen frame bnuffer
	p_vkEngine->RenderScene(m_offscreenInfo.cmdBuffer, m_offscreenInfo.descriptors.set, m_offscreenInfo.pipelineInfo.layout, m_offscreenInfo.pipelineInfo.pipeline);
		
	vkCmdEndRenderPass(m_offscreenInfo.cmdBuffer);
	
	VK_CHECK_RESULT(vkEndCommandBuffer(m_offscreenInfo.cmdBuffer));
}

void VulkanShadow::PrepareUBOBuffer()
{
	// vertex ubo
	m_offscreenInfo.uboBuffer.size = sizeof(OffscreenUbo);
	vkUtility.CreateBuffer(m_offscreenInfo.uboBuffer.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_offscreenInfo.uboBuffer.buffer, m_offscreenInfo.uboBuffer.memory);
}

void VulkanShadow::Update(CameraSystem *camera)
{	
	// calculate matrices based on the light source viewpoint
	m_offscreenInfo.uboData.projection = glm::perspective(glm::radians(camera->GetLightFOV()), 1.0f, camera->GetZNear(), camera->GetZFar());
	m_offscreenInfo.uboData.viewMatrix = glm::lookAt(camera->GetLightPosition(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_offscreenInfo.uboData.modelMatrix = glm::mat4(1.0f);

	vkUtility.MapBuffer<OffscreenUbo>(m_offscreenInfo.uboBuffer, m_offscreenInfo.uboData);
}

void VulkanShadow::Init()
{
	// create a semaphore for rendering
	m_offscreenInfo.semaphore = p_vkEngine->CreateSemaphore();

	PrepareOffscreenRenderpass();

	// create the cascade image layers and frame buffers
	PrepareDepthBuffer();

	PrepareUBOBuffer();
	PrepareOffscreenDescriptors();
	PrepareOffscreenPipeline();
}

void VulkanShadow::Destroy()
{

}

VkVertexInputBindingDescription VulkanShadow::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 1> VulkanShadow::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 1> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	return attrDescr;
}