#include "VulkanSkybox.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/VulkanModel.h"
#include "VulkanCore/VulkanIBL.h"
#include "Systems/camera_system.h"
#include "Engine/world.h"
#include "Engine/engine.h"

VulkanSkybox::VulkanSkybox(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory) :
	VulkanModule(utility, memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanSkybox::~VulkanSkybox()
{
}

void VulkanSkybox::PrepareSkyboxDescriptorSets()
{
	auto vkIBL = p_vkEngine->VkModule<VulkanIBL>();

	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },					// bindings for the UBO	
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }			// bindings for the colour image sampler
	};
	m_envCube.descriptors.AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorBufferInfo> buffInfo =
	{
		{ p_vkMemory->blockBuffer(m_envCube.uboBuffer.block_id), m_envCube.uboBuffer.offset, m_envCube.uboBuffer.size }
	};
	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ vkIBL->m_cubeImage.texSampler, vkIBL->m_cubeImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_envCube.descriptors.GenerateDescriptorSets(buffInfo.data(), imageInfo.data(), p_vkEngine->GetDevice());
}

void VulkanSkybox::PrepareSkyboxPipeline()
{
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>();

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

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 2;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	// render skybox where stencil = 0; i.e. were there is no geomtry
	depthInfo.stencilTestEnable = VK_TRUE;
	depthInfo.back.compareOp = VK_COMPARE_OP_NOT_EQUAL;
	depthInfo.back.failOp = VK_STENCIL_OP_KEEP;
	depthInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depthInfo.back.passOp = VK_STENCIL_OP_REPLACE;
	depthInfo.back.writeMask = 0x00;
	depthInfo.back.compareMask = 0x00;
	depthInfo.back.reference = 1;
	depthInfo.front = depthInfo.back;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_envCube.descriptors.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_envCube.pipeline.layout));

	// sahders for rendering full screen quad
	m_shader[0] = vkUtility->InitShaders("skybox/skybox-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("skybox/skybox-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	createInfo.layout = m_envCube.pipeline.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();		
	createInfo.subpass = 2;					// skybox drawn after lighting calculations
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_envCube.pipeline.pipeline));
}

void VulkanSkybox::GenerateSkyboxCmdBuffer(VkCommandBuffer cmdBuffer)
{
	// the cube map object data is loaded with all other models - it is assumed that the cubemap will default at index location 0 in the model map - TODO: ref cubemap data location in .json world data when implemented
	auto p_vkModel = p_vkEngine->VkModule<VulkanModel>();
	VulkanModel::ModelInfo model = p_vkModel->RequestModelInfo(0);

	VkDeviceSize offsets[1]{ p_vkModel->GetVertexOffset() };

	VkViewport viewport = vkUtility->InitViewPort(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 1.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkModel->GetVertexBuffer(), offsets);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkModel->GetIndexBuffer(), p_vkModel->GetIndexOffset(), VK_INDEX_TYPE_UINT32);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_envCube.pipeline.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_envCube.pipeline.layout, 0, 1, &m_envCube.descriptors.set, 0, NULL);
	vkCmdDrawIndexed(cmdBuffer, model.meshes[0].indexCount, 1, 0, 0, 0);

	viewport = vkUtility->InitViewPort(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
}

void VulkanSkybox::Update(int acc_time)
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// update skybox
	std::vector<SkyboxUbo> skyboxUbo(1);
	skyboxUbo[0].projection = camera->m_cameraInfo.projection * glm::mat4(glm::mat3(camera->m_cameraInfo.viewMatrix));
	skyboxUbo[0].modelMatrix = glm::mat4(1.0f);
	p_vkMemory->MapDataToSegment<SkyboxUbo>(m_envCube.uboBuffer, skyboxUbo);
}
void VulkanSkybox::Init()
{
	// skybox prepeartion 
	m_envCube.uboBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC | MemoryUsage::VK_UNIFORM_BUFFER, sizeof(SkyboxUbo));

	PrepareSkyboxDescriptorSets();
	PrepareSkyboxPipeline();
}

void VulkanSkybox::Destroy()
{

}