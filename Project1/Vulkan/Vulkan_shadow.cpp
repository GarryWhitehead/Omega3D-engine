#include "Vulkan_shadow.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/Vulkanrenderpass.h"
#include "VulkanCore/Vkdescriptors.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanModel.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/vulkan_terrain.h"
#include "Systems/camera_system.h"
#include "Systems/GraphicsSystem.h"
#include "Engine/engine.h"
#include <gtc/matrix_transform.hpp>
#include "Engine/World.h"
#include "ComponentManagers/LightComponentManager.h"
#include <algorithm>

VulkanShadow::VulkanShadow(VulkanEngine* engine, VkMemoryManager *memory) :
	VulkanModule(memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanShadow::~VulkanShadow()
{
	Destroy();
}

void VulkanShadow::PrepareShadowFrameBuffer()
{
	VkFormat format = VK_FORMAT_D32_SFLOAT_S8_UINT;

	auto p_light = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();
	uint32_t layerCount = p_light->GetLightCount();

	// prepare layered texture samplers 
	p_depthImage = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	p_depthImage->PrepareImageArray(format, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 1, layerCount);
	
	// prepare depth renderpass
	m_shadowInfo.renderpass = new VulkanRenderPass(p_vkEngine->GetDevice());
	m_shadowInfo.renderpass->AddAttachment(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, format);
	m_shadowInfo.renderpass->AddReference(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0);
	m_shadowInfo.renderpass->AddSubpassDependency(DependencyTemplate::TEMPLATE_DEPTH_STENCIL_SUBPASS_BOTTOM);
	m_shadowInfo.renderpass->AddSubpassDependency(DependencyTemplate::TEMPLATE_DEPTH_STENCIL_SUBPASS_FRAG);
	m_shadowInfo.renderpass->PrepareRenderPass(p_vkEngine->GetDevice());

	// create a frambuffer for each cascade layer
	m_shadowInfo.renderpass->PrepareFramebuffer(p_depthImage->imageView, SHADOWMAP_SIZE, SHADOWMAP_SIZE, p_vkEngine->GetDevice(), layerCount);
}

void VulkanShadow::PrepareShadowDescriptors()
{	
	m_shadowInfo.descriptors = new VkDescriptors(p_vkEngine->GetDevice());
	
	std::vector<VkDescriptors::LayoutBinding> uboLayout = 
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT }
	};
	m_shadowInfo.descriptors->AddDescriptorBindings(uboLayout);

	std::vector<VkDescriptorBufferInfo> buffInfo = 
	{
		{ p_vkMemory->blockBuffer(ssboBuffer.model.block_id), ssboBuffer.model.offset, ssboBuffer.model.size},
		{ p_vkMemory->blockBuffer(ssboBuffer.light.block_id), ssboBuffer.light.offset, ssboBuffer.light.size },
	};
	m_shadowInfo.descriptors->GenerateDescriptorSets(buffInfo.data(), nullptr);
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

	VkPipelineViewportStateCreateInfo viewportState = VulkanUtility::InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = VulkanUtility::InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
	rasterInfo.depthBiasEnable = VK_TRUE;

	VkPipelineMultisampleStateCreateInfo multiInfo = VulkanUtility::InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	VkPipelineColorBlendStateCreateInfo colorInfo = {};
	colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorInfo.attachmentCount = 0;

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = dynamicStates;
	dynamicInfo.dynamicStateCount = 3;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	// object index into mesh push
	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(PushConstant);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_shadowInfo.descriptors->layout;
	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_shadowInfo.pipelineInfo.layout));

	// load the shaders with tyexture samplers for material textures
	m_shadowInfo.shader[0] = VulkanUtility::InitShaders("Shadow/shadow-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_shadowInfo.shader[1] = VulkanUtility::InitShaders("Shadow/shadow-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());
	m_shadowInfo.shader[2] = VulkanUtility::InitShaders("Shadow/shadow-geom.spv", VK_SHADER_STAGE_GEOMETRY_BIT, p_vkEngine->GetDevice());

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
	createInfo.renderPass = m_shadowInfo.renderpass->renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_shadowInfo.pipelineInfo.pipeline));
}

void VulkanShadow::GenerateShadowCmdBuffer(VkCommandBuffer cmdBuffer)
{
	auto p_light = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();

	std::array<VkClearValue, 1> clearValues = {};
	clearValues[0].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	
	renderPassInfo.renderPass = m_shadowInfo.renderpass->renderpass;
	renderPassInfo.framebuffer = m_shadowInfo.renderpass->frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = SHADOWMAP_SIZE;
	renderPassInfo.renderArea.extent.height = SHADOWMAP_SIZE;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkViewport viewport = VulkanUtility::InitViewPort(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
	
	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	p_vkEngine->RenderScene(cmdBuffer, true);

	vkCmdEndRenderPass(cmdBuffer);
}


void VulkanShadow::Update(int acc_time)
{	
	auto p_light = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();

	// update light matricies
	{
		std::vector<SsboBufferLight> ssbo(1);

		for (uint32_t c = 0; c < p_light->GetLightCount(); ++c) {

			LightComponentManager::LightInfo info = p_light->GetLightData(c);

			// calculate matrices for each light based on the light source viewpoint
			glm::mat4 projection = glm::perspective(glm::radians(info.fov), 1.0f, zNear, zFar);
			//glm::mat4 projection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, zNear, zFar);
			glm::mat4 view = glm::lookAt(glm::vec3(info.pos), /*glm::vec3(info.target)*/ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

			glm::mat4 mat = projection * view;
			ssbo[0].mvp[c] = mat;
			p_light->UpdateLightViewMatrix(c, mat);
		}

		p_vkMemory->MapDataToSegment<SsboBufferLight>(ssboBuffer.light, ssbo);
	}

	// model matrices
	{
		std::vector<SsboBufferModel> ssbo(1);

		auto graphics = p_vkEngine->GetCurrentWorld()->RequestSystem<GraphicsSystem>();
		ssbo[0].modelMatrix = graphics->RequestTransformData();		// request updated transform data

		p_vkMemory->MapDataToSegment<SsboBufferModel>(ssboBuffer.model, ssbo);
	}
}

void VulkanShadow::Init()
{
	// create ubo buffer
	ssboBuffer.light = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(SsboBufferLight));
	ssboBuffer.model = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(SsboBufferModel));

	// create the cascade image layers and frame buffers
	PrepareShadowFrameBuffer();

	PrepareShadowDescriptors();
	PrepareShadowPipeline();
}

void VulkanShadow::Destroy()
{
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_shadowInfo.pipelineInfo.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_shadowInfo.pipelineInfo.layout, nullptr);

	delete m_shadowInfo.descriptors;
	delete m_shadowInfo.renderpass;
	delete p_depthImage;

	p_vkEngine = nullptr;
}

VkDescriptorSet& VulkanShadow::GetDescriptorSet()
{
	return m_shadowInfo.descriptors->set;
}

VkSampler& VulkanShadow::GetDepthSampler()
{
	return p_depthImage->texSampler;
}

VkImageView& VulkanShadow::GetDepthImageView()
{
	return p_depthImage->imageView;
}