#include "vulkan_terrain.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_shadow.h"
#include "VulkanCore/VulkanDeferred.h"
#include "utility/file_log.h"
#include "Systems/camera_system.h"
#include "Engine/World.h"
#include "Engine/engine.h"
#include "gli.hpp"
#include <math.h>

VulkanTerrain::VulkanTerrain(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory) :
	VulkanModule(utility, memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanTerrain::~VulkanTerrain()
{
}

void VulkanTerrain::LoadTerrainTextures()
{
	// create a repeating sampler for the terrain textures
	m_images.terrain.LoadTextureArray("assets/textures/terrain_texture_array.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 4.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine, p_vkMemory);
	
	// height map uses a mirrored sampler
	m_images.heightMap.LoadTexture("assets/textures/terrain_heightmap.ktx", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, 0.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R16_UNORM, p_vkEngine->GetCmdPool(), p_vkEngine, p_vkMemory);

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
	// terrain descriptors
	const int BIND_COUNT = 3;
	VkDescriptorPoolSize descrPoolSize[2] = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 3;										// fragment and tesselation con/eval shaders
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descrPoolSize[1].descriptorCount = 3;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 2;
	createInfo.pPoolSizes = descrPoolSize;
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_terrainInfo.descrInfo.pool));

	VkDescriptorSetLayoutBinding uboLayout = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);													// bindings for the UBO	
	VkDescriptorSetLayoutBinding samplerLayout1 = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);		// bindings for the height map image sampler
	VkDescriptorSetLayoutBinding samplerLayout2 = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);																								// bindings for texture array

	VkDescriptorSetLayoutBinding descrBind[] = { uboLayout, samplerLayout1, samplerLayout2 };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = BIND_COUNT;	
	layoutInfo.pBindings = descrBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_terrainInfo.descrInfo.layout));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_terrainInfo.descrInfo.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_terrainInfo.descrInfo.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_terrainInfo.descrInfo.set));

	VkDescriptorBufferInfo uboBuffInfo = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(m_terrainInfo.buffer.ubo.block_id), m_terrainInfo.buffer.ubo.offset, m_terrainInfo.buffer.ubo.size);										
	VkDescriptorImageInfo imageInfo1 = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_images.heightMap.imageView, m_images.heightMap.texSampler);		// height map texture sampler
	VkDescriptorImageInfo imageInfo2 = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_images.terrain.imageView, m_images.terrain.texSampler);			// texture array texture sampler

	VkWriteDescriptorSet writeDescrSet[BIND_COUNT] = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_terrainInfo.descrInfo.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_terrainInfo.descrInfo.set, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo1);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_terrainInfo.descrInfo.set, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo2);

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), BIND_COUNT, writeDescrSet, 0, nullptr);
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

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility->InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

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
	pipelineInfo.pSetLayouts = &m_terrainInfo.descrInfo.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_terrainInfo.pipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = vkUtility->InitShaders("terrain/land/terrain-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("terrain/land/terrain-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_shader[2] = vkUtility->InitShaders("terrain/land/terrain-tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	m_shader[3] = vkUtility->InitShaders("terrain/land/terrain-tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

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
}

void VulkanTerrain::GenerateTerrainCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	if (pipeline == VK_NULL_HANDLE) {
		vkCmdSetLineWidth(cmdBuffer, 1.0f);
	}

	VkDeviceSize bgOffsets[] = { m_terrainInfo.buffer.vertex.offset };

	// Terrain draw
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_terrainInfo.pipeline.pipeline : pipeline);	
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_terrainInfo.buffer.vertex.block_id), bgOffsets);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_terrainInfo.buffer.index.block_id), m_terrainInfo.buffer.index.offset, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_terrainInfo.pipeline.layout : layout, 0, 1, (pipeline == VK_NULL_HANDLE) ?  &m_terrainInfo.descrInfo.set : &set, 0, NULL);
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
	terrainUbo[0].disFactor = TESSELLATION_DISP_FACTOR;
	terrainUbo[0].tessFactor = TESSELLATION_FACTOR;
	terrainUbo[0].tessEdgeSize = TESSELLATION_EDGE_SIZE;
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

}

