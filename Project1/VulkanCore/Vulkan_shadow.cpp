#include "Vulkan_shadow.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanModel.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/vulkan_terrain.h"
#include "Systems/camera_system.h"
#include "Engine/engine.h"
#include <gtc/matrix_transform.hpp>
#include "Engine/World.h"
#include "ComponentManagers/LightComponentManager.h"
#include <algorithm>

VulkanShadow::VulkanShadow(VulkanEngine* engine, VulkanUtility *utility) :
	VulkanModule(utility),
	p_vkEngine(engine)
{
}

VulkanShadow::~VulkanShadow()
{
}

void VulkanShadow::PrepareShadowFrameBuffer()
{
	VkFormat format = VK_FORMAT_D32_SFLOAT_S8_UINT;

	auto p_lightManager = p_vkEngine->p_world->RequestComponentManager<LightComponentManager>(ComponentManagerId::CM_LIGHT_ID);
	int layerCount = p_lightManager->GetLightCount();

	// prepare layered texture samplers 
	m_depthImage.PrepareImageArray(format, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, layerCount, p_vkEngine);
	
	// prepare depth renderpass
	m_shadowInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, format);
	m_shadowInfo.renderpass.AddReference(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0);
	m_shadowInfo.renderpass.PrepareRenderPass(p_vkEngine->GetDevice());

	// create a frambuffer for each cascade layer
	m_shadowInfo.renderpass.PrepareFramebuffer(m_depthImage.imageView, SHADOWMAP_SIZE, SHADOWMAP_SIZE, p_vkEngine->GetDevice(), layerCount);
}

void VulkanShadow::PrepareShadowDescriptors()
{
	// prepare descriptor sets for depth sampling - will be used in conjuction with other vulkan module sets
	std::array<VkDescriptorPoolSize, 1> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_shadowInfo.descriptors.pool));

	std::array<VkDescriptorSetLayoutBinding, 1> uboLayout = {};
	uboLayout[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT);
	
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.pBindings = uboLayout.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(uboLayout.size());

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_shadowInfo.descriptors.layout));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_shadowInfo.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_shadowInfo.descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_shadowInfo.descriptors.set));
	
	// depth descriptor - vertex ubo and fragment shader
	std::array<VkDescriptorBufferInfo, 1> buffInfo = {};
	buffInfo[0] = vkUtility->InitBufferInfoDescriptor(m_shadowInfo.uboBuffer.buffer, 0, m_shadowInfo.uboBuffer.size);

	std::array<VkWriteDescriptorSet, 1> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_shadowInfo.descriptors.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &buffInfo[0]);

	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanShadow::PrepareShadowPipeline()
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

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE);
	rasterInfo.depthBiasEnable = VK_TRUE;

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility->InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	VkPipelineColorBlendStateCreateInfo colorInfo = {};
	colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorInfo.attachmentCount = 0;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR /*VK_DYNAMIC_STATE_DEPTH_BIAS*/ };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = dynamicStates;
	dynamicInfo.dynamicStateCount = 2;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_shadowInfo.descriptors.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_shadowInfo.pipelineInfo.layout));

	// load the shaders with tyexture samplers for material textures
	m_shadowInfo.shader[0] = vkUtility->InitShaders("Shadow/shadow-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shadowInfo.shader[1] = vkUtility->InitShaders("Shadow/shadow-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_shadowInfo.shader[2] = vkUtility->InitShaders("Shadow/shadow-geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT);

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 3;
	createInfo.pStages = m_shadowInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_shadowInfo.pipelineInfo.layout;
	createInfo.renderPass = m_shadowInfo.renderpass.renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_shadowInfo.pipelineInfo.pipeline));
}

void VulkanShadow::GenerateShadowCmdBuffer(VkCommandBuffer cmdBuffer)
{
	// create a semaphore to ensure that the shadow map is updated before generating on screen commands
	m_shadowInfo.semaphore = p_vkEngine->CreateSemaphore();

	std::array<VkClearValue, 7> clearValues = {};

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	
	renderPassInfo.renderPass = m_shadowInfo.renderpass.renderpass;
	renderPassInfo.framebuffer = m_shadowInfo.renderpass.frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = SHADOWMAP_SIZE;
	renderPassInfo.renderArea.extent.height = SHADOWMAP_SIZE;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();
	
	// try and reduce artifacts
	//vkCmdSetDepthBias(cmdBuffer, biasConstant, 0.0f, biasSlope);

	VkViewport viewport = vkUtility->InitViewPort(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// draw scene into each cascade layer
	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	p_vkEngine->RenderScene(cmdBuffer, m_shadowInfo.descriptors.set, m_shadowInfo.pipelineInfo.layout, m_shadowInfo.pipelineInfo.pipeline);
	vkCmdEndRenderPass(cmdBuffer);
}


void VulkanShadow::Update(int acc_time)
{	
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID);
	auto p_light = p_vkEngine->p_world->RequestComponentManager<LightComponentManager>(ComponentManagerId::CM_LIGHT_ID);
	auto camera = p_vkEngine->p_world->RequestSystem<CameraSystem>(SystemId::CAMERA_SYSTEM_ID);

	for(uint32_t c = 0; c < p_light->GetLightCount(); ++c) {

		LightComponentManager::LightInfo info = p_light->GetLightData(c);

		// calculate matrices for each light based on the light source viewpoint
		glm::mat4 projection = glm::perspective(glm::radians(info.fov), 1.0f, camera->GetZNear(), camera->GetZFar());
		glm::mat4 view = glm::lookAt(glm::vec3(info.pos), glm::vec3(info.target), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 model = glm::mat4(1.0f);

		m_shadowInfo.uboData.mvp[c] = projection * view * model;
	}

	m_shadowInfo.uboBuffer.MapBuffer<UboLayout>(m_shadowInfo.uboData, p_vkEngine->GetDevice());
}

void VulkanShadow::Init()
{
	// create ubo buffer
	m_shadowInfo.uboBuffer.CreateBuffer(sizeof(UboLayout), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_vkEngine);

	// create the cascade image layers and frame buffers
	PrepareShadowFrameBuffer();

	PrepareShadowDescriptors();
	PrepareShadowPipeline();
}

void VulkanShadow::Destroy()
{

}