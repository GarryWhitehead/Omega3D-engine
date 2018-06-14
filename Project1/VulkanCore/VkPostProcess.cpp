#include "VkPostProcess.h"
#include "VulkanCore/VkDescriptors.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanDeferred.h"
#include "Systems/camera_system.h"
#include "Engine/World.h"
#include <gtc/matrix_transform.hpp>

VkPostProcess::VkPostProcess(VulkanEngine *engine, VkMemoryManager *memory) :
	VulkanModule(memory),
	p_vkEngine(engine)
{
	Init();
}

VkPostProcess::~VkPostProcess()
{
}

void VkPostProcess::GenerateCmdBuffer(VkCommandBuffer cmdBuffer)
{
	VkViewport viewport = VulkanUtility::InitViewPort(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	VkDeviceSize offsets[1]{ m_vertices.offset };

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fogInfo.pipelineInfo.pipeline);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertices.block_id), offsets);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_fogInfo.pipelineInfo.layout, 0, 1, &m_fogInfo.descriptors->set, 0, NULL);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indices.block_id), m_indices.offset, VK_INDEX_TYPE_UINT32);

	// draw as a full-screen quad
	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
}

void VkPostProcess::PrepareFullscreenQuad()
{
	// prepare vertices
	std::vector<Vertex> vertices;
	vertices.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f }, } );
	vertices.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f }, } );
	vertices.push_back({ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f }, } );
	vertices.push_back({ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }, } );

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

	// map vertices
	m_vertices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(Vertex) * vertices.size());
	p_vkMemory->MapDataToSegment<Vertex>(m_vertices, vertices);

	// map indices
	m_indices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * indices.size());
	p_vkMemory->MapDataToSegment<uint32_t>(m_indices, indices);
}

void VkPostProcess::PrepareDescriptors()
{
	m_fogInfo.descriptors = new VkDescriptors(p_vkEngine->GetDevice());
	
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>();

	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },					// bindings for the UBO	
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }					// bindings for the offscreen colour image sampler
	};
	m_fogInfo.descriptors->AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorBufferInfo> buffInfo =
	{
		{ p_vkMemory->blockBuffer(m_fogInfo.uboBufferVS.block_id), m_fogInfo.uboBufferVS.offset, m_fogInfo.uboBufferVS.size },
		{ p_vkMemory->blockBuffer(m_fogInfo.uboBufferFS.block_id), m_fogInfo.uboBufferFS.offset, m_fogInfo.uboBufferFS.size }
	};
	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ vkDeferred->GetOffscreenSampler(), vkDeferred->GetOffscreenImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_fogInfo.descriptors->GenerateDescriptorSets(buffInfo.data(), imageInfo.data());
}

void VkPostProcess::PreparePipeline()
{
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

	VkPipelineViewportStateCreateInfo viewportState =VulkanUtility::InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo =VulkanUtility::InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo =VulkanUtility::InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

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
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_fogInfo.descriptors->layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_fogInfo.pipelineInfo.layout));

	// sahders for rendering full screen quad
	m_fogInfo.shader[0] =VulkanUtility::InitShaders("Post-Process/fog/volumetric_fog-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_fogInfo.shader[1] =VulkanUtility::InitShaders("Post-Process/fog/volumetric_fog-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_fogInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_fogInfo.pipelineInfo.layout;
	createInfo.renderPass = p_vkEngine->GetFinalRenderPass();
	createInfo.subpass = 0;						
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_fogInfo.pipelineInfo.pipeline));
}

void VkPostProcess::Update(int acc_time)
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// upload constant fragment ubo data
	std::vector<FogUboFS> uboFS(1);
	uboFS[0].fogDensity = FOG_DENSITY;
	uboFS[0].rayDir = RAY_DIRECTION;
	uboFS[0].sunDir = SUN_DIRECTION;
	uboFS[0].enableFog = p_vkEngine->drawFog();

	p_vkMemory->MapDataToSegment<FogUboFS>(m_fogInfo.uboBufferFS, uboFS);

	// vertex shader
	std::vector<FogUboVS> uboVS(1);
	uboVS[0].projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
	uboVS[0].viewMatrix = glm::mat4(1.0f);
	uboVS[0].modelMatrix = glm::mat4(1.0f);
	uboVS[0].camModelView = camera->m_cameraInfo.viewMatrix * glm::mat4(1.0f);

	p_vkMemory->MapDataToSegment<FogUboVS>(m_fogInfo.uboBufferVS, uboVS);
}

void VkPostProcess::Init()
{
	// create ubo buffers
	m_fogInfo.uboBufferVS = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(FogUboVS));
	m_fogInfo.uboBufferFS = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(FogUboFS));

	// create pipeline
	PrepareFullscreenQuad();
	PrepareDescriptors();
	PreparePipeline();
}

void VkPostProcess::Destroy()
{
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_fogInfo.pipelineInfo.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_fogInfo.pipelineInfo.layout, nullptr);

	delete m_fogInfo.descriptors;

	p_vkEngine = nullptr;
}

// shader setup
VkVertexInputBindingDescription VkPostProcess::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 2> VkPostProcess::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 2> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(Vertex, uv);

	return attrDescr;
}
