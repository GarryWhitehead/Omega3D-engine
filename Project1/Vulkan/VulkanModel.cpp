#include "VulkanModel.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VkDescriptors.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/Vulkan_shadow.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "VulkanCore/VulkanDeferred.h"
#include "Systems/camera_system.h"
#include "Systems/GraphicsSystem.h"
#include "Engine/engine.h"
#include "Engine/World.h"

VulkanModel::VulkanModel(VulkanEngine *engine, MemoryAllocator *memory) :
	VulkanModule(memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanModel::~VulkanModel()
{
	Destroy();
}

void VulkanModel::Destroy()
{
	// delete pipelines
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_pipelineInfo.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_pipelineInfo.layout, nullptr);

	// delete materials - textures and descriptors
	for (auto& mat : m_materials) {

		delete mat.descriptor;
		
		if (mat.texture.diffuse != nullptr)
			delete mat.texture.diffuse;
		if (mat.texture.normal != nullptr)
			delete mat.texture.normal;
		if (mat.texture.roughness != nullptr)
			delete mat.texture.roughness;
		if (mat.texture.diffuse != nullptr)
			delete mat.texture.metallic;
		if (mat.texture.ao != nullptr)
			delete mat.texture.ao;
	}

	// delete dummy textures
	delete m_dummyTexture.diffuse;
	delete m_dummyTexture.normal;
	delete m_dummyTexture.roughness;
	delete m_dummyTexture.metallic;
	delete m_dummyTexture.ao;
}

void VulkanModel::ImportDummyTextures()
{
	std::string path("assets/models/textures/");

	m_dummyTexture.diffuse = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_dummyTexture.normal = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_dummyTexture.metallic = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_dummyTexture.roughness = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_dummyTexture.ao = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());

	m_dummyTexture.diffuse->LoadTexture(path + "dummy_diffuse.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
	m_dummyTexture.normal->LoadTexture(path + "dummy_normal.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
	m_dummyTexture.roughness->LoadTexture(path + "dummy_roughness.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
	m_dummyTexture.metallic->LoadTexture(path + "dummy_metallic.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
	m_dummyTexture.ao->LoadTexture(path + "dummy_ao.ktx", VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
}

void VulkanModel::PrepareMaterialDescriptorSets(Material *material)
{
	material->descriptor = new VkDescriptors(p_vkEngine->GetDevice());

	// we are using the same layout for all models - only the textures will differ
	std::vector<VkDescriptors::LayoutBinding> layouts =
	{
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },		// diffuse
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },		// normal
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },		// roughness
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },		// metallic
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }			// ao
	};
	material->descriptor->AddDescriptorBindings(layouts);

	// set all the textures to dummy images and override if the image contains a certain texture
	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ m_dummyTexture.diffuse->texSampler, m_dummyTexture.diffuse->imageView,  vk::ImageLayout::eShaderReadOnlyOptimal },				
		{ m_dummyTexture.normal->texSampler, m_dummyTexture.normal->imageView,  vk::ImageLayout::eShaderReadOnlyOptimal },
		{ m_dummyTexture.roughness->texSampler, m_dummyTexture.roughness->imageView,  vk::ImageLayout::eShaderReadOnlyOptimal },
		{ m_dummyTexture.metallic->texSampler, m_dummyTexture.metallic->imageView,  vk::ImageLayout::eShaderReadOnlyOptimal },
		{ m_dummyTexture.ao->texSampler, m_dummyTexture.ao->imageView,  vk::ImageLayout::eShaderReadOnlyOptimal }
	};

	// if a certain texture is found within the material, then override the dummy texture
	if (material->matTypes != (int)MaterialTexture::TEX_NONE) {								// check that this model actually has texture info

		if (material->matTypes & (int)MaterialTexture::TEX_DIFF) {

			imageInfo[0] = VkDescriptorImageInfo({ material->texture.diffuse->texSampler, material->texture.diffuse->imageView, vk::ImageLayout::eShaderReadOnlyOptimal });		// diffuse texture - #1
		}
		if (material->matTypes & (int)MaterialTexture::TEX_NORM) {

			imageInfo[1] = VkDescriptorImageInfo({ material->texture.normal->texSampler, material->texture.normal->imageView, vk::ImageLayout::eShaderReadOnlyOptimal });		// normal texture - #2
		}
		if (material->matTypes & (int)MaterialTexture::TEX_ROUGHNESS) {

			imageInfo[2] = VkDescriptorImageInfo({ material->texture.roughness->texSampler, material->texture.roughness->imageView, vk::ImageLayout::eShaderReadOnlyOptimal });	// roughness texture - #3
		}
		if (material->matTypes & (int)MaterialTexture::TEX_METALLIC) {

			imageInfo[3] = VkDescriptorImageInfo({ material->texture.metallic->texSampler, material->texture.metallic->imageView, vk::ImageLayout::eShaderReadOnlyOptimal });	// metallic texture - #4
		}
		if (material->matTypes & (int)MaterialTexture::TEX_AO) {

			imageInfo[4] = VkDescriptorImageInfo({ material->texture.ao->texSampler, material->texture.ao->imageView, vk::ImageLayout::eShaderReadOnlyOptimal });				// ao texture - #5
		}
	}
	
	// vertex ubo -the same for all materials
	std::vector<VkDescriptorBufferInfo> uboBuffInfo =
	{
		{ p_vkMemory->blockBuffer(m_ssboBuffer.block_id), m_ssboBuffer.offset, m_ssboBuffer.size }
	};

	// generate descriptor sets dependent on the textures used by this material
	material->descriptor->GenerateDescriptorSets(uboBuffInfo.data(), imageInfo.data());
}

void VulkanModel::PreparePipeline()
{
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>();
	
	// scene graphics pipeline
	ModelVertex vertex;
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

	VkPipelineRasterizationStateCreateInfo rasterInfo = VulkanUtility::InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

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

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 2;

	// object index into mesh push
	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(uint32_t);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// material properties push
	VkPushConstantRange pushConstant2 = {};
	pushConstant2.size = sizeof(MaterialProperties);
	pushConstant2.offset = sizeof(uint32_t);
	pushConstant2.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPushConstantRange pushConstantArray[] = { pushConstant, pushConstant2 };
	
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_materials[0].descriptor->layout;
	pipelineInfo.pPushConstantRanges = pushConstantArray;
	pipelineInfo.pushConstantRangeCount = 2;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_pipelineInfo.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = VulkanUtility::InitShaders("Model/model-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_shader[1] = VulkanUtility::InitShaders("Model/model-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());

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
	createInfo.layout = m_pipelineInfo.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();
	createInfo.subpass = 0;												// render to G-buffer pass
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipelineInfo.pipeline));
}

void VulkanModel::GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, bool drawShadow)
{
	auto p_meshManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<MeshComponentManager>();

	// TODO::tidy this code - dont allow vulkan model direct access to mesh data
	for (uint32_t m = 0; m < p_meshManager->m_data.meshIndex.size(); ++m) {			

		uint32_t index = p_meshManager->m_data.meshIndex[m];
	
		for (uint32_t i = 0; i < m_modelInfo[index].meshes.size(); i++) {		// the number of meshes within this particular model - each mesh has its own material

			// get the model data 
			auto& model = m_modelInfo[index];

			// and the material for this particluar model from the store
			uint32_t matIndex;
			if (!p_meshManager->m_data.materialName[m].empty()) {

				matIndex = FindMaterialIndex(p_meshManager->m_data.materialName[m]);
			}
			else {
				matIndex = model.meshes[i].materialIndex;
			}
			assert(matIndex < m_materials.size());

			// the model matrix indices 
			uint32_t objIndex = m;			// TODO:: look up the object id between mesh and transform data 

			// we can either draw using the shadow or model pipeline
			if (drawShadow) {		// use the shadow pipeline

				auto shadow = p_vkEngine->VkModule<VulkanShadow>();

				// try and reduce artifacts
				vkCmdSetDepthBias(cmdBuffer, VulkanShadow::biasConstant, 0.0f, VulkanShadow::biasSlope);

				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow->GetPipeline());
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadow->GetPipelineLayout(), 0, 1, &shadow->GetDescriptorSet(), 0, NULL);

				VulkanShadow::PushConstant push;
				push.useModelIndex = 1;					// inform the shadow shader to add the model transform to the light matrix	
				push.modelIndex = objIndex;
				vkCmdPushConstants(cmdBuffer, shadow->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(VulkanShadow::PushConstant), &push);

			}
			else {

				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.pipeline);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.layout, 0, 1, &m_materials[matIndex].descriptor->set, 0, NULL);

				vkCmdPushConstants(cmdBuffer, m_pipelineInfo.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &objIndex);															// object index into transform buffer
				vkCmdPushConstants(cmdBuffer, m_pipelineInfo.layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(MaterialProperties), &m_materials[matIndex].properties);		// push material info per mesh
			}

			// bind vertex data at offset into buffer
			VkDeviceSize offset[]{ m_modelInfo[index].verticesOffset + m_vertexBuffer.offset };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertexBuffer.block_id), offset);

			// bind index data derived from face indices - draw each face with one draw call as material differ between each and we will be pushing the material data per draw call
			vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indexBuffer.block_id), m_indexBuffer.offset + (model.meshes[i].indexBase * sizeof(uint32_t)), VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(cmdBuffer, model.meshes[i].indexCount, 1, 0, 0, 0);
		}
	}
}

void VulkanModel::ProcessMeshes()
{
	uint32_t indexBase = 0;
	uint32_t vertexBase = 0;
	std::vector<ModelVertex> vertices;
	std::vector<uint32_t> indices;

	auto p_meshManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<MeshComponentManager>();

	m_modelInfo.resize(p_meshManager->m_models.size());

	for (uint32_t m = 0; m < p_meshManager->m_models.size(); ++m) {

		m_modelInfo[m].meshes.resize(p_meshManager->m_models[m].meshData.size());
		m_modelInfo[m].verticesOffset = vertices.size() * sizeof(ModelVertex);

		for (uint32_t c = 0; c < p_meshManager->m_models[m].meshData.size(); ++c) {

			// material for each mesh - first check whether to use user-defined material rather than default
			std::string mat = p_meshManager->m_models[m].meshData[c].material;
			m_modelInfo[m].meshes[c].materialIndex = FindMaterialIndex(mat);

			MeshComponentManager::OMFMesh mesh = p_meshManager->m_models[m].meshData[c];

			// upload vertices
			
			for (uint32_t i = 0; i < mesh.posData.size(); ++i) {
				
				ModelVertex vertex;
				vertex.pos = mesh.posData[i];
				vertex.pos.y = -vertex.pos.y;

				if (!mesh.uvData.empty()) {
					vertex.uv = mesh.uvData[i];
				}
				else {
					vertex.uv = glm::vec2(0.0f, 0.0f);
				}

				vertex.normal = mesh.normData[i];

				if (!mesh.colorData.empty()) {
					vertex.colour = mesh.colorData[i];
				}
				else {
					vertex.colour = glm::vec3(1.0f, 1.0f, 1.0f);
				}
				vertices.push_back(vertex);
			}
		}
				// add the bone info
				/*for (uint32_t b = 0; b < MAX_VERTEX_BONES; ++b) {
				vertex.boneId[b] = m_vertexBoneData[i].boneId[b];
				vertex.boneWeigthts[b] = m_vertexBoneData[i].weights[b];
				}*/

		m_modelInfo[m].indicesOffset = indices.size() * sizeof(uint32_t);

		// retrieve the face indices data for this model
		for (uint32_t c = 0; c < p_meshManager->m_models[m].meshData.size(); ++c) {
		
			uint32_t count = 0;
			m_modelInfo[m].meshes[c].indexBase = indexBase;

			MeshComponentManager::OMFMesh mesh = p_meshManager->m_models[m].meshData[c];

			for (uint32_t f = 0; f < mesh.faceData.size(); ++f) {						// all faces will be rendered with one draw command

				for (uint32_t v = 0; v < mesh.faceData[f].indices.size(); ++v) {		// this will be size = 3 as all data must be triangulated

					indices.push_back(mesh.faceData[f].indices[v]);
					++count;
				}
			}

			indexBase += count;
			m_modelInfo[m].meshes[c].indexCount = count;
		}
	}	

	// create local device buffers to map index and vertex data too
	m_vertexBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(ModelVertex) * vertices.size());
	p_vkMemory->MapDataToSegment<ModelVertex>(m_vertexBuffer, vertices);

	m_indexBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * indices.size());
	p_vkMemory->MapDataToSegment<uint32_t>(m_indexBuffer, indices);
}

uint8_t VulkanModel::FindMaterialIndex(std::string matName)
{
	uint32_t index = 0;
	for (auto& mat : m_materials) {

		if (mat.name == matName) {

			return index;
		}
		++index;
	}
	return UINT8_MAX;
}

void VulkanModel::ProcessMaterials()
{
	auto p_meshManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<MeshComponentManager>();
	
	for (auto& model : p_meshManager->m_models) {

		for (uint32_t c = 0; c < model.materials.size(); ++c) {

			MeshComponentManager::OMFMaterial mat = model.materials[c];
			Material pipelineMat;
			pipelineMat.name = mat.name;

			// Get the material properties
			pipelineMat.properties.metallic = mat.Color.metallic;
			pipelineMat.properties.roughness = mat.Color.roughness;
			pipelineMat.properties.ao = mat.Color.ao;

			if (mat.hasTexture()) {
		
				if (mat.hasTexture(MaterialType::ALBEDO_TYPE)) {
					pipelineMat.texture.diffuse = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
					LoadMaterialTexture(mat, MaterialType::ALBEDO_TYPE, pipelineMat.texture.diffuse);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_DIFF;
				}
				if (mat.hasTexture(MaterialType::NORMAL_TYPE)) {
					pipelineMat.texture.normal = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
					LoadMaterialTexture(mat, MaterialType::NORMAL_TYPE, pipelineMat.texture.normal);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_NORM;
				}
				if (mat.hasTexture(MaterialType::ROUGHNESS_TYPE)) {
					pipelineMat.texture.roughness = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
					LoadMaterialTexture(mat, MaterialType::ROUGHNESS_TYPE, pipelineMat.texture.roughness);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_ROUGHNESS;
				}
				if (mat.hasTexture(MaterialType::METALLIC_TYPE)) {
					pipelineMat.texture.metallic = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
					LoadMaterialTexture(mat, MaterialType::METALLIC_TYPE, pipelineMat.texture.metallic);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_METALLIC;
				}
				if (mat.hasTexture(MaterialType::AO_TYPE)) {
					pipelineMat.texture.ao = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
					LoadMaterialTexture(mat, MaterialType::AO_TYPE, pipelineMat.texture.ao);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_AO;
				}
			}
			else {
				pipelineMat.matTypes = (int)MaterialTexture::TEX_NONE;
			}

			m_materials.push_back(pipelineMat);
		}
	}
}

void VulkanModel::LoadMaterialTexture(MeshComponentManager::OMFMaterial &material, MaterialType type, VulkanTexture *texture)
{

	std::string filename;

	filename = material.GetMaterialType(type);

	// check whether original file is of tga format
	size_t pos = filename.find(".tga");
	if (pos != std::string::npos) {

		filename = filename.substr(0, pos);			
		filename = filename + ".ktx";
	}
	pos = filename.find(".jpg");
	if (pos != std::string::npos) {

		filename = filename.substr(0, pos);			
		filename = filename + ".ktx";
	}
	pos = filename.find(".png");
	if (pos != std::string::npos) {

		filename = filename.substr(0, pos);
		filename = filename + ".ktx";
	}
	
	filename = "assets/models/textures/" + filename;
	texture->LoadTexture(filename, VK_SAMPLER_ADDRESS_MODE_REPEAT, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);

	return;
}

void VulkanModel::Init()
{
	ImportDummyTextures();
	
	// process meshes and trasfer to local device
	ProcessMaterials();
	ProcessMeshes();
	
	// create uniform buffer for camera perspective info 
	m_ssboBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(SsboLayout));

	// prepare descriptor sets for meshes and materials	
	for (auto& mat : m_materials) {

		PrepareMaterialDescriptorSets(&mat);
	}

	PreparePipeline();
}

void VulkanModel::Update(int acc_time)
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// ssbo buffer for transform matrices 
	std::vector<SsboLayout> ssbo(1);

	ssbo[0].projection = camera->m_cameraInfo.projection;
	ssbo[0].viewMatrix = camera->m_cameraInfo.viewMatrix;
	
	auto graphics = p_vkEngine->GetCurrentWorld()->RequestSystem<GraphicsSystem>();
	ssbo[0].modelMatrix = graphics->RequestTransformData();		// request updated transform data
	
	p_vkMemory->MapDataToSegment<SsboLayout>(m_ssboBuffer, ssbo); 

	// update bone animation transforms for each model
	/* for (auto& model : p_modelManager->m_colladaModels) {
		
		//model.UpdateModelAnimation();

		for (uint32_t c = 0; c < model.m_boneTransforms.size(); ++c) {

			ubo.boneTransform[c] = model.m_boneTransforms[c];
		}
	} */	
}

// vertex attributes for model vertex buffer

VkVertexInputBindingDescription VulkanModel::ModelVertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(ModelVertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

std::array<VkVertexInputAttributeDescription, 6> VulkanModel::ModelVertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 6> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(ModelVertex, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(ModelVertex, uv);

	// Vertex layout 2: normal
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(ModelVertex, normal);

	// Vertex layout 4: colour
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(ModelVertex, colour);

	// Vertex layout 5: bone weights
	attrDescr[4].binding = 0;
	attrDescr[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attrDescr[4].location = 4;
	attrDescr[4].offset = offsetof(ModelVertex, boneWeigthts);

	// Vertex layout 6: bone ids
	attrDescr[5].binding = 0;
	attrDescr[5].format = VK_FORMAT_R32G32B32_SINT;
	attrDescr[5].location = 5;
	attrDescr[5].offset = offsetof(ModelVertex, boneId);

	return attrDescr;
}
