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

VulkanShadow::VulkanShadow(VulkanEngine* engine, VulkanUtility *utility, VkMemoryManager *memory) :
	VulkanModule(utility, memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanShadow::~VulkanShadow()
{
}

void VulkanShadow::PrepareShadowFrameBuffer()
{
	VkFormat format = VK_FORMAT_D32_SFLOAT_S8_UINT;

	auto p_lightManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();
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
	std::vector<VkDescriptors::LayoutBinding> uboLayout = 
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT }
	};
	m_shadowInfo.descriptors.AddDescriptorBindings(uboLayout);

	std::vector<VkDescriptorBufferInfo> buffInfo = 
	{
		{ p_vkMemory->blockBuffer(m_shadowInfo.uboBuffer.block_id), m_shadowInfo.uboBuffer.offset, m_shadowInfo.uboBuffer.size}
	};
	m_shadowInfo.descriptors.GenerateDescriptorSets(buffInfo.data(), nullptr, p_vkEngine->GetDevice());
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

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

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

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_shadowInfo.pipelineInfo.layout));

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

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_shadowInfo.pipelineInfo.pipeline));
}

void VulkanShadow::GenerateShadowCmdBuffer()
{
	// if we have already generated a commnad buffer but are re-drawing, then free the present buffer
	if (m_cmdBuffer != VK_NULL_HANDLE) {

		vkFreeCommandBuffers(p_vkEngine->GetDevice(), p_vkEngine->GetCmdPool(), 1, &m_cmdBuffer);
	}

	// create a semaphore to ensure that the shadow map is updated before generating on screen commands
	m_shadowInfo.semaphore = p_vkEngine->CreateSemaphore();

	std::array<VkClearValue, 7> clearValues = {};

	m_cmdBuffer = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetCmdPool());

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
	vkCmdSetViewport(m_cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, 0);
	vkCmdSetScissor(m_cmdBuffer, 0, 1, &scissor);

	// draw scene into each cascade layer
	vkCmdBeginRenderPass(m_cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	p_vkEngine->RenderScene(m_cmdBuffer, m_shadowInfo.descriptors.set, m_shadowInfo.pipelineInfo.layout, m_shadowInfo.pipelineInfo.pipeline);
	vkCmdEndRenderPass(m_cmdBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(m_cmdBuffer));
}


void VulkanShadow::Update(int acc_time)
{	
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>();
	auto p_light = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	std::vector<UboLayout> ubo(1);

	for(uint32_t c = 0; c < p_light->GetLightCount(); ++c) {

		LightComponentManager::LightInfo info = p_light->GetLightData(c);

		// calculate matrices for each light based on the light source viewpoint
		glm::mat4 projection = glm::perspective(glm::radians(info.fov), 1.0f, camera->GetZNear(), camera->GetZFar());
		glm::mat4 view = glm::lookAt(glm::vec3(info.pos), glm::vec3(info.target), glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 model = glm::mat4(1.0f);

		ubo[0].mvp[c] = projection * view * model;
	}

	p_vkMemory->MapDataToSegment<UboLayout>(m_shadowInfo.uboBuffer, ubo);
}

void VulkanShadow::Init()
{
	// create ubo buffer
	m_shadowInfo.uboBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(UboLayout));

	// create the cascade image layers and frame buffers
	PrepareShadowFrameBuffer();

	PrepareShadowDescriptors();
	PrepareShadowPipeline();
}

void VulkanShadow::Destroy()
{

}