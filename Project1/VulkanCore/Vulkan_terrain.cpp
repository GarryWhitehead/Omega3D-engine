#include "vulkan_terrain.h"
#include "VulkanCore/VkDescriptors.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_shadow.h"
#include "VulkanCore/VulkanDeferred.h"
#include "utility/file_log.h"
#include "Systems/camera_system.h"
#include "Engine/World.h"
#include "Engine/engine.h"
#include "gli.hpp"
#include <math.h>

VulkanTerrain::VulkanTerrain(VulkanEngine *engine, VkMemoryManager *memory) :
	VulkanModule(memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanTerrain::~VulkanTerrain()
{
	Destroy();
}

void VulkanTerrain::LoadTerrainTextures()
{
	// create a repeating sampler for the terrain textures
	m_images.terrain = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_images.terrain->LoadTextureArray("assets/textures/terrain_texture_array.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 4.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
	
	// height map uses a mirrored sampler
	m_images.heightMap = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_images.heightMap->LoadTexture("assets/textures/terrain_heightmap.ktx", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, 0.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R16_UNORM, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);

	// load heightmap and transfer data to new buffer
	gli::texture2d tex(gli::load("assets/textures/terrain_heightmap.ktx"));

	if (tex.empty()) {
		*g_filelog << "Critical error importing heightmap data! Unable to open file \n";
		exit(EXIT_FAILURE);
	}

	// copy into new buffer
	m_heightmapInfo.imageDim = static_cast<uint32_t>(tex.extent().x);	// only one channel
	uint32_t size = m_heightmapInfo.imageDim * m_heightmapInfo.imageDim;
	m_heightmapInfo.data = new uint16_t[size];
	memcpy(m_heightmapInfo.data, tex.data(), tex.size());
}

void VulkanTerrain::PrepareTerrainDescriptorSets()
{
	m_terrainInfo.descrInfo = new VkDescriptors(p_vkEngine->GetDevice());

	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT },												// bindings for the UBO	
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },		// bindings for the height map image sampler
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }																									// bindings for texture array
	};
	m_terrainInfo.descrInfo->AddDescriptorBindings(layoutBind);
	
	std::vector<VkDescriptorBufferInfo> uboBuffInfo = 
	{
		{ p_vkMemory->blockBuffer(m_terrainInfo.buffer.ubo.block_id), m_terrainInfo.buffer.ubo.offset, m_terrainInfo.buffer.ubo.size }
	};

	std::vector<VkDescriptorImageInfo> imageInfo = 
	{
		{ m_images.heightMap->texSampler, m_images.heightMap->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// height map texture sampler
		{ m_images.terrain->texSampler, m_images.terrain->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }				// texture array texture sampler
	};
	m_terrainInfo.descrInfo->GenerateDescriptorSets(uboBuffInfo.data(), imageInfo.data());
}

void VulkanTerrain::PreparePipeline()
{
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>();

	// tesselation pipeline
	TerrainUtility::Vertex vertex;
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
	assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;										// used for tesselation shader draw
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = VulkanUtility::InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = VulkanUtility::InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = VulkanUtility::InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	// colour attachment required for each colour buffer
	std::array<VkPipelineColorBlendAttachmentState, 8> colorAttach = {};
	colorAttach[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[0].blendEnable = VK_FALSE;
	colorAttach[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[1].blendEnable = VK_FALSE;
	colorAttach[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[2].blendEnable = VK_FALSE;
	colorAttach[3].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[3].blendEnable = VK_FALSE;
	colorAttach[4].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[4].blendEnable = VK_FALSE;
	colorAttach[5].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[5].blendEnable = VK_FALSE;
	colorAttach[6].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[6].blendEnable = VK_FALSE;
	colorAttach[7].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[7].blendEnable = VK_FALSE;

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

	// enable stencil test - to ensure that the skybox is rendered correctly
	depthInfo.stencilTestEnable = VK_TRUE;
	depthInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthInfo.back.failOp = VK_STENCIL_OP_REPLACE;
	depthInfo.back.depthFailOp = VK_STENCIL_OP_REPLACE;
	depthInfo.back.passOp = VK_STENCIL_OP_REPLACE;
	depthInfo.back.writeMask = 0xff;
	depthInfo.back.compareMask = 0xff;
	depthInfo.back.reference = 1;
	depthInfo.front = depthInfo.back;

	VkPipelineTessellationStateCreateInfo tessInfo = {};
	tessInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessInfo.patchControlPoints = 4;		//	patch quad

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_terrainInfo.descrInfo->layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_terrainInfo.pipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = VulkanUtility::InitShaders("terrain/land/terrain-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_shader[1] = VulkanUtility::InitShaders("terrain/land/terrain-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());
	m_shader[2] = VulkanUtility::InitShaders("terrain/land/terrain-tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, p_vkEngine->GetDevice());
	m_shader[3] = VulkanUtility::InitShaders("terrain/land/terrain-tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, p_vkEngine->GetDevice());

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = static_cast<uint32_t>(m_shader.size());
	createInfo.pStages = m_shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.pTessellationState = &tessInfo;
	createInfo.layout = m_terrainInfo.pipeline.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();			// render into the offscreen buffer
	createInfo.subpass = 0;											// G buffer pass
	createInfo.basePipelineIndex = -1;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_terrainInfo.pipeline.pipeline));

	// wireframe draw pipeline
	rasterInfo.polygonMode = VK_POLYGON_MODE_LINE;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_terrainInfo.wirePipeline));
}

void VulkanTerrain::GenerateTerrainCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	if (pipeline == VK_NULL_HANDLE) {
		vkCmdSetLineWidth(cmdBuffer, 1.0f);
	}

	VkDeviceSize bgOffsets[] = { m_terrainInfo.buffer.vertex.offset };

	// Terrain draw
	if (pipeline == VK_NULL_HANDLE) {
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (p_vkEngine->drawWireframe()) ? m_terrainInfo.wirePipeline : m_terrainInfo.pipeline.pipeline);
	}
	else {
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);		// use alternate pipeline for draw
	}

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_terrainInfo.buffer.vertex.block_id), bgOffsets);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_terrainInfo.buffer.index.block_id), m_terrainInfo.buffer.index.offset, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_terrainInfo.pipeline.layout : layout, 0, 1, (pipeline == VK_NULL_HANDLE) ?  &m_terrainInfo.descrInfo->set : &set, 0, NULL);
	vkCmdDrawIndexed(cmdBuffer, m_terrainInfo.buffer.indexCount, 1, 0, 0, 0);
}

void VulkanTerrain::PrepareTerrainData()
{
	TerrainUtility *p_utility = new TerrainUtility();

	// generate vertices and uv data
	std::vector<TerrainUtility::Vertex> vertices;
	p_utility->GenerateVertices(PATCH_SIZE, UV_SCALE, vertices);

	// generate normals from height map using sorbel operator
	const float xKernal[] = { 1.0f, 0.0f, -1.0f, 2.0f , 0.0f, -2.0f, 1.0f, 0.0f, -1.0f };
	const float yKernal[] = { 1.0f, 2.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -2.0f, -1.0f };
	
	for (uint32_t x = 0; x < PATCH_SIZE; ++x) {

		for (uint32_t y = 0; y < PATCH_SIZE; ++y) {

			float Gx = 0.0f;
			float Gz = 0.0f;
			uint32_t kIndex = 0;

			for (int32_t hx = -1; hx <= 1; ++hx) {

				for (int32_t hy = -1; hy <= 1; ++hy) {

					float heightPixel = GetHeightmapPixel(x + hx, y + hy);
					Gx += xKernal[kIndex] * heightPixel;					// apply x kernal to pixel
					Gz += yKernal[kIndex] * heightPixel;					// and y kernal
					++kIndex;
				}
			}
			glm::vec3 normal(Gx, 0.0f, Gz);
			
			// the co-efficent can be used to regulate the "bumpiness" of the map - <1.0 enhance - >1.0 smooth
			// calculate gradient using SQRT(1 - Gx^2 - Gy^2)
			normal.y = 0.2f * sqrt(1.0f - Gx * Gx - Gz * Gz);
			
			// convert to 0..1 space
			vertices[x + y * PATCH_SIZE].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
		}			
	}

	// generate the indices for the patch quads
	m_terrainInfo.buffer.indexCount = ((PATCH_SIZE - 1) * (PATCH_SIZE - 1)) * 4;

	std::vector<uint32_t> indices;
	p_utility->GenerateIndices(PATCH_SIZE, indices);

	// now map to device memory
	// vertex
	m_terrainInfo.buffer.vertex = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, vertices.size() * sizeof(TerrainUtility::Vertex));
	p_vkMemory->MapDataToSegment<TerrainUtility::Vertex>(m_terrainInfo.buffer.vertex, vertices);

	//index
	m_terrainInfo.buffer.index = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, indices.size() * sizeof(uint32_t));
	p_vkMemory->MapDataToSegment<uint32_t>(m_terrainInfo.buffer.index, indices);

}

float VulkanTerrain::GetHeightmapPixel(uint32_t x, uint32_t y)
{
	uint32_t scale = m_heightmapInfo.imageDim / PATCH_SIZE;		// normalise heighmap image dimensions to the patch
	
	// get interpolated position from scaled image
	glm::ivec2 interPos = glm::ivec2(x, y) * glm::ivec2(scale);
	interPos.x = std::max(0, std::min(interPos.x, (int)m_heightmapInfo.imageDim - 1));		// set to zero if negative;
	interPos.y = std::max(0, std::min(interPos.y, (int)m_heightmapInfo.imageDim - 1));
	interPos /= glm::ivec2(scale);													// adjust back after interpolation

	// return pixel from calculated psoition
	return *(m_heightmapInfo.data + (interPos.x + interPos.y * m_heightmapInfo.imageDim) * scale) / (65535.0f);
}

void VulkanTerrain::Update(int acc_time)
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// update terrain tesselation ubo
	std::vector<TerrainUbo> terrainUbo(1);
	terrainUbo[0].projection = camera->m_cameraInfo.projection;
	terrainUbo[0].viewMatrix = camera->m_cameraInfo.viewMatrix;
	terrainUbo[0].modelMatrix = glm::mat4(1.0f);
	terrainUbo[0].screenDim = glm::vec2(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH());

	// constant tessellation values
	terrainUbo[0].disFactor = p_vkEngine->displacementFactor();
	terrainUbo[0].tessFactor = p_vkEngine->tesselationFactor();
	terrainUbo[0].tessEdgeSize = p_vkEngine->tessEdgeSize();
	p_vkMemory->MapDataToSegment<TerrainUbo>(m_terrainInfo.buffer.ubo, terrainUbo);
}

void VulkanTerrain::Init()
{
	// check that the device supports tesselation shaders - no point in continuing if this isn't the case
	//if (p_vkEngine->m_device.features.tessellationShader == VK_FALSE) {

	//	*g_filelog << "Critical error initialising  Vulkan terrain module! GPU device does not support tessellation shaders.";
	//	exit(EXIT_FAILURE);
	//}

	// load model data - loads materials, then the vertex information, creates the device memory buffers and uploads the data
	this->LoadTerrainTextures();

	m_terrainInfo.buffer.ubo = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(TerrainUbo));
	this->PrepareTerrainData();

	this->PrepareTerrainDescriptorSets();
	this->PreparePipeline();
}

void VulkanTerrain::Destroy()
{
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_terrainInfo.pipeline.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_terrainInfo.pipeline.layout, nullptr);
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_terrainInfo.wirePipeline, nullptr);

	delete m_terrainInfo.descrInfo;
	delete m_images.terrain;
	delete m_images.heightMap;

	p_vkEngine = nullptr;
}

