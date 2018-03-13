#include "vulkan_terrain.h"
#include "VulkanCore/vulkan_scene.h"
#include "utility/file_log.h"
#include "Systems/camera_system.h"
#include "gli.hpp"
#include <math.h>

VulkanTerrain::VulkanTerrain()
{
}

VulkanTerrain::VulkanTerrain(VulkanScene *vulkanScene) :
	VulkanUtility(vulkanScene),
	p_vulkanScene(vulkanScene)
{
}

VulkanTerrain::~VulkanTerrain()
{
}

void VulkanTerrain::LoadTerrainTextures()
{
	//m_images.skybox = LoadTexture("skybox", VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_COMPARE_OP_ALWAYS, 16, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R8G8B8A8_UNORM, p_vulkanScene->m_cmdPool);

	// create a repeating sampler for the terrain textures
	m_images.terrain = LoadTextureArray("assets/textures/terrain_texture_array.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_COMPARE_OP_ALWAYS, 4, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R8G8B8A8_UNORM, p_vulkanScene->m_cmdPool);
	
	// height map uses a mirrored sampler
	m_images.heightMap = LoadTexture("assets/textures/terrain_heightmap.ktx", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_COMPARE_OP_ALWAYS, 0, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R16_UNORM, p_vulkanScene->m_cmdPool);	

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

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vulkanScene->m_device.device, &createInfo, nullptr, &m_terrainInfo.descrInfo.pool));

	VkDescriptorSetLayoutBinding uboLayout = InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);													// bindings for the UBO	
	VkDescriptorSetLayoutBinding samplerLayout1 = InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);		// bindings for the height map image sampler
	VkDescriptorSetLayoutBinding samplerLayout2 = InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);																								// bindings for texture array

	VkDescriptorSetLayoutBinding descrBind[] = { uboLayout, samplerLayout1, samplerLayout2 };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = BIND_COUNT;	
	layoutInfo.pBindings = descrBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vulkanScene->m_device.device, &layoutInfo, nullptr, &m_terrainInfo.descrInfo.layout));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_terrainInfo.descrInfo.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_terrainInfo.descrInfo.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vulkanScene->m_device.device, &allocInfo, &m_terrainInfo.descrInfo.set));

	VkDescriptorBufferInfo uboBuffInfo = InitBufferInfoDescriptor(m_terrainInfo.buffer.ubo.buffer, 0, m_terrainInfo.buffer.ubo.size);											// Buffer information - UBO buffer size and location
	VkDescriptorImageInfo imageInfo1 = InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_images.heightMap.imageView, m_images.heightMap.m_tex_sampler);		// height map texture sampler
	VkDescriptorImageInfo imageInfo2 = InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_images.terrain.imageView, m_images.terrain.m_tex_sampler);								// texture array texture sampler

	VkWriteDescriptorSet writeDescrSet[BIND_COUNT] = {};
	writeDescrSet[0] = InitDescriptorSet(m_terrainInfo.descrInfo.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	writeDescrSet[1] = InitDescriptorSet(m_terrainInfo.descrInfo.set, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo1);
	writeDescrSet[2] = InitDescriptorSet(m_terrainInfo.descrInfo.set, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo2);

	vkUpdateDescriptorSets(p_vulkanScene->m_device.device, BIND_COUNT, writeDescrSet, 0, nullptr);
}

void VulkanTerrain::PrepareSkyboxDescriptorSets()
{
	// skybox descriptors
	const int BIND_COUNT = 2;
	VkDescriptorPoolSize descrPoolSize[2] = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;										
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descrPoolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = BIND_COUNT;
	createInfo.pPoolSizes = descrPoolSize;
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vulkanScene->m_device.device, &createInfo, nullptr, &m_skyboxInfo.descrInfo.pool));

	VkDescriptorSetLayoutBinding uboLayout = InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);						// bindings for the UBO	
	VkDescriptorSetLayoutBinding samplerLayout = InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// bindings for the colour image sampler
	
	VkDescriptorSetLayoutBinding descrBind[] = { uboLayout, samplerLayout };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = BIND_COUNT;
	layoutInfo.pBindings = descrBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vulkanScene->m_device.device, &layoutInfo, nullptr, &m_skyboxInfo.descrInfo.layout));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_skyboxInfo.descrInfo.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_skyboxInfo.descrInfo.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vulkanScene->m_device.device, &allocInfo, &m_skyboxInfo.descrInfo.set));

	VkDescriptorBufferInfo uboBuffInfo = InitBufferInfoDescriptor(m_skyboxInfo.buffer.ubo.buffer, 0, m_skyboxInfo.buffer.ubo.size);											// Buffer information - UBO buffer size and location
	VkDescriptorImageInfo imageInfo = InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_images.skybox.imageView, m_images.skybox.m_tex_sampler);		// height map texture sampler

	VkWriteDescriptorSet writeDescrSet[BIND_COUNT] = {};
	writeDescrSet[0] = InitDescriptorSet(m_skyboxInfo.descrInfo.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	writeDescrSet[1] = InitDescriptorSet(m_skyboxInfo.descrInfo.set, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);

	vkUpdateDescriptorSets(p_vulkanScene->m_device.device, BIND_COUNT, writeDescrSet, 0, nullptr);
}

void VulkanTerrain::PreparePipeline()
{
	// tesselation pipeline
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
	assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;										// used for tesselation shader draw
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = InitViewPort(p_vulkanScene->m_viewport.viewPort, p_vulkanScene->m_viewport.scissor, p_vulkanScene->m_surface.extent.width, p_vulkanScene->m_surface.extent.height);

	VkPipelineRasterizationStateCreateInfo rasterInfo = InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	VkPipelineColorBlendAttachmentState colorAttach = {};
	colorAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorInfo = {};
	colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorInfo.logicOpEnable = VK_FALSE;
	colorInfo.attachmentCount = 1;
	colorInfo.pAttachments = &colorAttach;

	VkDynamicState states[] = { VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 1;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineTessellationStateCreateInfo tessInfo = {};
	tessInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
	tessInfo.patchControlPoints = 4;		//	patch quad

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_terrainInfo.descrInfo.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vulkanScene->m_device.device, &pipelineInfo, nullptr, &m_terrainInfo.pipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = InitShaders("terrain/terrain-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = InitShaders("terrain/terrain-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	m_shader[2] = InitShaders("terrain/terrain-tesc.spv", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
	m_shader[3] = InitShaders("terrain/terrain-tese.spv", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);

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
	createInfo.renderPass = p_vulkanScene->m_renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vulkanScene->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_terrainInfo.pipeline.pipeline));

	// skybox pipeline derivative
	//assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	//depthInfo.depthTestEnable = VK_FALSE;

	//m_shader[0] = InitShaders("assets/shaders/skybox/skybox-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	//m_shader[1] = InitShaders("assets/shaders/skybox/skybox-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	//createInfo.pTessellationState = nullptr;
	//createInfo.layout = m_skyboxInfo.pipeline.layout;
	//createInfo.stageCount = 2;

	//createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;											
	//createInfo.basePipelineHandle = m_terrainInfo.pipeline.pipeline;
	//VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vulkanScene->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_skyboxInfo.pipeline.pipeline));
}

void VulkanTerrain::GenerateCmdBuffers()
{
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = p_vulkanScene->CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	m_cmdBuffer.resize(p_vulkanScene->m_frameBuffer.size());

	for (uint32_t c = 0; c < m_cmdBuffer.size(); ++c) {

		m_cmdBuffer[c] = CreateCmdBuffer(VK_PRIMARY, VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vulkanScene->m_cmdPool);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = p_vulkanScene->m_frameBuffer[c];
		renderPassInfo.renderPass = p_vulkanScene->m_renderpass;
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent.width = p_vulkanScene->m_surface.extent.width;
		renderPassInfo.renderArea.extent.height = p_vulkanScene->m_surface.extent.height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_cmdBuffer[c], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetLineWidth(m_cmdBuffer[c], 1.0f);

		// draw the skybox first
		VkDeviceSize bgOffsets[] = { 0 };
		//vkCmdBindVertexBuffers(m_cmdBuffer[c], 0, 1, &m_skyboxInfo.buffer.vertex.buffer, bgOffsets);
		//vkCmdBindIndexBuffer(m_cmdBuffer[c], m_skyboxInfo.buffer.index.buffer, 0, VK_INDEX_TYPE_UINT32);

		//vkCmdBindPipeline(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxInfo.pipeline.pipeline);
		//vkCmdBindDescriptorSets(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_skyboxInfo.pipeline.layout, 0, 1, &m_skyboxInfo.descrInfo.set, 0, NULL);
		//vkCmdDrawIndexed(m_cmdBuffer[c], m_skyboxInfo.buffer.indexCount, 1, 0, 0, 0);

		// Terrain draw
		vkCmdBindVertexBuffers(m_cmdBuffer[c], 0, 1, &m_terrainInfo.buffer.vertex.buffer, bgOffsets);
		vkCmdBindIndexBuffer(m_cmdBuffer[c], m_terrainInfo.buffer.index.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindPipeline(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_terrainInfo.pipeline.pipeline);
		vkCmdBindDescriptorSets(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_terrainInfo.pipeline.layout, 0, 1, &m_terrainInfo.descrInfo.set, 0, NULL);
		vkCmdDrawIndexed(m_cmdBuffer[c], m_terrainInfo.buffer.indexCount, 1, 0, 0, 0);

		// end of the command definitions for this render pass so tell the GPU
		vkCmdEndRenderPass(m_cmdBuffer[c]);
		VK_CHECK_RESULT(vkEndCommandBuffer(m_cmdBuffer[c]));
	}
}

void VulkanTerrain::PrepareTerrainData()
{
	// generate vertices and uv data
	const uint32_t vertCount = PATCH_SIZE * PATCH_SIZE;
	std::vector<Vertex> vertices;
	vertices.resize(vertCount);

	for (uint32_t x = 0; x < PATCH_SIZE; ++x) {

		for (uint32_t y = 0; y < PATCH_SIZE; ++y) {

			uint32_t index = x + y * PATCH_SIZE;
			vertices[index].pos.x = x * WX + WX / 2.0f - (float)PATCH_SIZE * WX / 2.0f;
			vertices[index].pos.z = y * WY + WY / 2.0f - (float)PATCH_SIZE * WY / 2.0f;
			vertices[index].pos.y = 0.0f;
			vertices[index].uv = glm::vec2(x / (float)PATCH_SIZE, y / (float)PATCH_SIZE) * 1.0f;
		}
	}

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
			normal.y = 0.5f * sqrt(1.0f - Gx * Gx - Gz * Gz);
			
			// convert to 0..1 space
			vertices[x + y * PATCH_SIZE].normal = glm::normalize(normal * glm::vec3(2.0f, 1.0f, 2.0f));
		}			
	}

	// generate the indices for the patch quads
	uint32_t width = PATCH_SIZE - 1;
	m_terrainInfo.buffer.indexCount = width * width * 4;

	std::vector<uint32_t> indices;
	indices.resize(m_terrainInfo.buffer.indexCount);

	for (uint32_t x = 0; x < width; ++x) {

		for (uint32_t y = 0; y < width; ++y) {

			uint32_t index = (x + y * width) * 4;
			indices[index] = (x + y * PATCH_SIZE);				// top-left
			indices[index + 1] = indices[index] + PATCH_SIZE;	// bottom-left
			indices[index + 2] = indices[index + 1] + 1;		//	bottom-right
			indices[index + 3] = indices[index] + 1;			// top-right
		}
	}

	// now map to device memory
	// vertex
	m_terrainInfo.buffer.vertex.size = vertCount * sizeof(Vertex);
	CreateBuffer(m_terrainInfo.buffer.vertex.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_terrainInfo.buffer.vertex.buffer, m_terrainInfo.buffer.vertex.memory);
	MapBuffer<Vertex>(m_terrainInfo.buffer.vertex, vertices);

	//index
	m_terrainInfo.buffer.index.size = m_terrainInfo.buffer.indexCount * sizeof(uint32_t);
	CreateBuffer(m_terrainInfo.buffer.index.size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_terrainInfo.buffer.index.buffer, m_terrainInfo.buffer.index.memory);
	MapBuffer<uint32_t>(m_terrainInfo.buffer.index, indices);
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
	return *(m_heightmapInfo.data + (interPos.x + interPos.y * m_heightmapInfo.imageDim) * scale) / (65536.0f);
}

void VulkanTerrain::PrepareUBOBuffer()
{
	m_terrainInfo.buffer.ubo.size = sizeof(TerrainUbo);
	CreateBuffer(m_terrainInfo.buffer.ubo.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_terrainInfo.buffer.ubo.buffer, m_terrainInfo.buffer.ubo.memory);
}

void VulkanTerrain::Update(CameraSystem *camera)
{
	// update terrain tesselation ubo
	std::vector<TerrainUbo> uboData;

	TerrainUbo ubo;
	ubo.projection = camera->m_cameraInfo.projection;
	ubo.viewMatrix = camera->m_cameraInfo.viewMatrix;
	ubo.modelMatrix = glm::mat4(1.0f);
	ubo.screenDim = glm::vec2(p_vulkanScene->m_surface.extent.width, p_vulkanScene->m_surface.extent.height);

	// constant tessellation values
	ubo.disFactor = TESSELLATION_DISP_FACTOR;
	ubo.tessFactor = TESSELLATION_FACTOR;
	ubo.tessEdgeSize = TESSELLATION_EDGE_SIZE;
	uboData.push_back(ubo);

	MapBuffer<TerrainUbo>(m_terrainInfo.buffer.ubo, uboData);
}

void VulkanTerrain::Init()
{
	// check that the device supports tesselation shaders - no point in continuing if this isn't the case
	if (p_vulkanScene->m_device.features.tessellationShader == VK_FALSE) {

		*g_filelog << "Critical error initialising  Vulkan terrain module! GPU device does not support tessellation shaders.";
		exit(EXIT_FAILURE);
	}

	// load model data - loads materials, then the vertex information, creates the device memory buffers and uploads the data
	this->LoadTerrainTextures();

	this->PrepareUBOBuffer();
	this->PrepareTerrainData();

	this->PrepareTerrainDescriptorSets();
	//this->PrepareSkyboxDescriptorSets();
	this->PreparePipeline();
	
	this->GenerateCmdBuffers();
}

VkVertexInputBindingDescription VulkanTerrain::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 3> VulkanTerrain::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: uv
	std::array<VkVertexInputAttributeDescription, 3> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	// Vertex layout 1: normal
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(Vertex, uv);

	// Vertex layout 2: colour
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(Vertex, normal);

	return attrDescr;
}