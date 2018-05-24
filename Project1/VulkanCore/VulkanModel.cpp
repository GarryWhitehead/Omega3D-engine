#include "VulkanModel.h"
#include "VulkanCore/VulkanEngine.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "VulkanCore/VulkanDeferred.h"
#include "Systems/camera_system.h"
#include "Systems/GraphicsSystem.h"
#include "Engine/engine.h"
#include "Engine/World.h"

VulkanModel::VulkanModel(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory) :
	VulkanModule(utility, memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanModel::~VulkanModel()
{
}

void VulkanModel::Destroy()
{

}

void VulkanModel::PrepareMeshDescriptorSet()
{
	std::array<VkDescriptorPoolSize, 1> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descrPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_animInfo.mesh.descriptors.pool));

	// scene descriptor layout
	VkDescriptorSetLayoutBinding uboLayout = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	VkDescriptorSetLayoutBinding sceneBind[] = { uboLayout };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = sceneBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_animInfo.mesh.descriptors.layout));

	// Create descriptor set for meshes
	VkDescriptorSetLayout layouts[] = { m_animInfo.mesh.descriptors.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_animInfo.mesh.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_animInfo.mesh.descriptors.set));

	VkDescriptorBufferInfo uboBuffInfo = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(m_ssboBuffer.block_id), m_ssboBuffer.offset, m_ssboBuffer.size);

	std::array<VkWriteDescriptorSet, 1> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_animInfo.mesh.descriptors.set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &uboBuffInfo);
	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanModel::PrepareMaterialDescriptorPool(uint32_t materialCount)
{
	if (materialCount == 0) {
		materialCount = 1;			// stop vulkan complaining about pool size being zero
	}

	// create descriptor pool for all models and materials
	std::array<VkDescriptorPoolSize, 5> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = materialCount;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// diffuse
	descrPoolSize[1].descriptorCount = materialCount;
	descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// normal
	descrPoolSize[2].descriptorCount = materialCount;
	descrPoolSize[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// roughness
	descrPoolSize[3].descriptorCount = materialCount;
	descrPoolSize[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// metallic
	descrPoolSize[4].descriptorCount = materialCount;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = materialCount + 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_animInfo.material.descriptors.pool));
}

void VulkanModel::PrepareMaterialDescriptorLayouts()
{
	// create a layout for each possible map arrangment - these will be assigned to models later
	uint32_t bindCount = 0;
	std::vector<VkDescriptorSetLayoutBinding> layoutBind;
	layoutBind.push_back(vkUtility->InitLayoutBinding(bindCount++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));		// diffuse
	
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	layoutInfo.pBindings = layoutBind.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_animInfo.descriptors.diffLayout));
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_animInfo.descriptors.nomapLayout));			// no map and diffuse only use the same layout for convienence

	layoutBind.push_back(vkUtility->InitLayoutBinding(bindCount++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));		// diffuse + normal	
	layoutInfo.pBindings = layoutBind.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_animInfo.descriptors.diffnormLayout));

	layoutBind.push_back(vkUtility->InitLayoutBinding(bindCount++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));		
	layoutBind.push_back(vkUtility->InitLayoutBinding(bindCount++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));		// diffuse + normal + roughness + metallic (PBR)
	layoutInfo.pBindings = layoutBind.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_animInfo.descriptors.pbrLayout));
}

void VulkanModel::PrepareMaterialDescriptorSets(Material *material)
{
	std::vector<VkDescriptorSetLayout> layouts;
	std::vector<VkDescriptorImageInfo> imageInfo;
	
	if (material->matTypes & (int)MaterialTexture::TEX_NONE) {

		layouts.push_back(m_animInfo.descriptors.diffLayout);
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.nomap.imageView, material->texture.nomap.texSampler));		// no map - using dummy texture as vulkan requires an imageview
	}
	else if (material->matTypes & (int)MaterialTexture::TEX_PBR) {

		layouts.push_back(m_animInfo.descriptors.pbrLayout);
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.diffuse.imageView, material->texture.diffuse.texSampler));		// diffuse texture
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.normal.imageView, material->texture.normal.texSampler));			// normal texture
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.roughness.imageView, material->texture.roughness.texSampler));	// roughness texture
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.metallic.imageView, material->texture.metallic.texSampler));		// metallic texture
	}
	else if (material->matTypes & (int)MaterialTexture::TEX_DIFF) {

		layouts.push_back(m_animInfo.descriptors.diffLayout);
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.diffuse.imageView, material->texture.diffuse.texSampler));		// diffuse texture

		if (material->matTypes & (int)MaterialTexture::TEX_NORM) {

			layouts[0] = (m_animInfo.descriptors.diffnormLayout);
			imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.normal.imageView, material->texture.normal.texSampler));		// normal texture
		}
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_animInfo.material.descriptors.pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();
	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &material->descrSet));
		
	std::vector<VkWriteDescriptorSet> writeDescrSet;
	writeDescrSet.resize(imageInfo.size());
	for (uint32_t c = 0; c < imageInfo.size(); ++c) {

		writeDescrSet[c] = vkUtility->InitDescriptorSet(material->descrSet, c, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[c]);
	}

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanModel::PreparePipeline()
{
	// pipeline for materials containing diffuse and normal maps
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

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

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

	// entity index into mesh push
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
	std::vector<VkDescriptorSetLayout> descrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.diffnormLayout };
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = static_cast<uint32_t>(descrLayouts.size());
	pipelineInfo.pSetLayouts = descrLayouts.data();
	pipelineInfo.pPushConstantRanges = pushConstantArray;
	pipelineInfo.pushConstantRangeCount = 2;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_animInfo.matPipelines.diffNorm.layout));

	// load the shaders with tyexture samplers for material textures
	m_animInfo.shader[0] = vkUtility->InitShaders("skinning/mesh-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_animInfo.shader[1] = vkUtility->InitShaders("skinning/mesh_DIFFNORM-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_animInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_animInfo.matPipelines.diffNorm.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();
	createInfo.subpass = 0;												// render to G-buffer pass
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.diffNorm.pipeline));

	createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;											// inform the API that we are using a pipeline derivative
	createInfo.basePipelineHandle = m_animInfo.matPipelines.diffNorm.pipeline;

	// diffuse map only pipeline
	std::vector<VkDescriptorSetLayout> dndescrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.diffLayout };
	pipelineInfo.pSetLayouts = dndescrLayouts.data();
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_animInfo.matPipelines.diff.layout));

	createInfo.layout = m_animInfo.matPipelines.diff.layout;
	m_animInfo.shader[1] = vkUtility->InitShaders("skinning/mesh_DIFF-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.diff.pipeline));

	// PBR texture pipeline
	std::vector<VkDescriptorSetLayout> pbrdescrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.pbrLayout };
	pipelineInfo.pSetLayouts = pbrdescrLayouts.data();
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_animInfo.matPipelines.pbr.layout));

	createInfo.layout = m_animInfo.matPipelines.pbr.layout;
	m_animInfo.shader[1] = vkUtility->InitShaders("skinning/mesh_PBR-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.pbr.pipeline));

	// no-material map pipeline
	std::vector<VkDescriptorSetLayout> nomapdescrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.diffLayout };		// no material map shader has a sampler bound but not used
	pipelineInfo.pSetLayouts = nomapdescrLayouts.data();
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_animInfo.matPipelines.nomap.layout));

	createInfo.layout = m_animInfo.matPipelines.nomap.layout;
	m_animInfo.shader[1] = vkUtility->InitShaders("skinning/mesh_NOMAP-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.nomap.pipeline));
}

void VulkanModel::GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	auto p_meshManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<MeshComponentManager>();

	// TODO::tidy this code - dont allow vulkan model direct access to mesh data
	for (uint32_t m = 0; m < p_meshManager->m_data.meshIndex.size(); ++m) {			

		uint32_t index = p_meshManager->m_data.meshIndex[m];

		// bind vertex data at offset into buffer
		VkDeviceSize offset[]{ m_modelInfo[index].verticesOffset + m_vertexBuffer.offset };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertexBuffer.block_id), offset);
		
		for (uint32_t i = 0; i < m_modelInfo[index].meshes.size(); i++) {		// the number of meshes within this particular model - each mesh has its own material

			auto& model = m_modelInfo[index];

			uint32_t matIndex;
			if (!p_meshManager->m_data.materialName[m].empty()) {
				matIndex = FindMaterialIndex(p_meshManager->m_data.materialName[m]);
			}
			else {
				matIndex = model.meshes[i].materialIndex;
			}
			assert(matIndex < m_materials.size());

			std::array<VkDescriptorSet, 2> descrSets = { m_animInfo.mesh.descriptors.set, m_materials[matIndex].descrSet };
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_materials[matIndex].pipeline : pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_materials[matIndex].layout : layout, 0, (pipeline == VK_NULL_HANDLE) ? static_cast<uint32_t>(descrSets.size()) : 1, (pipeline == VK_NULL_HANDLE) ? descrSets.data() : &set, 0, NULL);

			// bind index data derived from face indices - draw each face with one draw call as material differ between each and we will be pushing the material data per draw call
			vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indexBuffer.block_id), m_indexBuffer.offset + (model.meshes[i].indexBase * sizeof(uint32_t)), VK_INDEX_TYPE_UINT32);

			if (pipeline == VK_NULL_HANDLE) {
				uint32_t objIndex = m;			// TODO:: look up the object id between mesh and transform data 
				vkCmdPushConstants(cmdBuffer, m_materials[matIndex].layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &objIndex);														// object index into transform buffer
				vkCmdPushConstants(cmdBuffer,m_materials[matIndex].layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t), sizeof(MaterialProperties), &m_materials[matIndex].properties);		// push material info per mesh
			}
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

			if (!mat.hasTexture()) {
				pipelineMat.matTypes |= (int)MaterialTexture::TEX_NONE;
				 LoadMaterialTexture(mat, MaterialType::NO_TYPE, pipelineMat.texture.nomap);
			}
			else {
				if (mat.hasTexture(MaterialType::ALBEDO_TYPE)) {
					LoadMaterialTexture(mat, MaterialType::ALBEDO_TYPE, pipelineMat.texture.diffuse);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_DIFF;
				}
				if (mat.hasTexture(MaterialType::NORMAL_TYPE)) {
					LoadMaterialTexture(mat, MaterialType::NORMAL_TYPE, pipelineMat.texture.normal);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_NORM;
				}
				if (mat.hasTexture(MaterialType::ROUGHNESS_TYPE)) {
					LoadMaterialTexture(mat, MaterialType::ROUGHNESS_TYPE, pipelineMat.texture.roughness);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_PBR;
				}
				if (mat.hasTexture(MaterialType::METALLIC_TYPE)) {
					LoadMaterialTexture(mat, MaterialType::METALLIC_TYPE, pipelineMat.texture.metallic);
					pipelineMat.matTypes |= (int)MaterialTexture::TEX_PBR;
				}

			}
			m_materials.push_back(pipelineMat);
		}
	}
}

void VulkanModel::LoadMaterialTexture(MeshComponentManager::OMFMaterial &material, MaterialType type, VulkanTexture &texture)
{

	std::string filename;

	if (type == MaterialType::NO_TYPE) {

		filename = "dummy.ktx";
	}
	else {

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
	}

	filename = "assets/models/textures/" + filename;
	texture.LoadTexture(filename, VK_SAMPLER_ADDRESS_MODE_REPEAT, 16, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, p_vkEngine->GetCmdPool(), p_vkEngine, p_vkMemory);

	return;
}

void VulkanModel::AddPipelineDataToModels()
{
	for (auto& mat : m_materials) {

		if (mat.matTypes & (int)MaterialTexture::TEX_NONE) {

			mat.layout = m_animInfo.matPipelines.nomap.layout;
			mat.pipeline = m_animInfo.matPipelines.nomap.pipeline;
		}
		else if (mat.matTypes & (int)MaterialTexture::TEX_PBR) {

			mat.layout = m_animInfo.matPipelines.pbr.layout;
			mat.pipeline = m_animInfo.matPipelines.pbr.pipeline;
		}
		else if (mat.matTypes & (int)MaterialTexture::TEX_DIFF) {

			if (mat.matTypes & (int)MaterialTexture::TEX_NORM) {

				mat.layout = m_animInfo.matPipelines.diffNorm.layout;
				mat.pipeline = m_animInfo.matPipelines.diffNorm.pipeline;
			}
			else {

				mat.layout = m_animInfo.matPipelines.diff.layout;
				mat.pipeline = m_animInfo.matPipelines.diff.pipeline;
			}
		}
	}
}

void VulkanModel::Init()
{
	// process meshes and trasfer to local device
	ProcessMaterials();
	ProcessMeshes();
	
	// create uniform buffer for camera perspective info 
	m_ssboBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(SsboLayout));

	// prepare descriptor sets for meshes and materials
	PrepareMaterialDescriptorPool(m_materials.size());
	PrepareMeshDescriptorSet();
	PrepareMaterialDescriptorLayouts();

	for (auto& mat : m_materials) {

		PrepareMaterialDescriptorSets(&mat);
	}

	PreparePipeline();
	AddPipelineDataToModels();
}

void VulkanModel::Update(int acc_time)
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// ssbo buffer for transform matrices 
	std::vector<SsboLayout> ssbo(1);

	ssbo[0].projection = camera->m_cameraInfo.projection;
	ssbo[0].viewMatrix = camera->m_cameraInfo.viewMatrix;
	
	auto graphics = p_vkEngine->RequestGraphicsSystem();
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
