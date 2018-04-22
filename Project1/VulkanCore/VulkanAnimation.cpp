#include "VulkanAnimation.h"
#include "VulkanCore/VulkanEngine.h"
#include "Engine/ModelResourceManager.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "VulkanCore/VulkanDeferred.h"
#include "Systems/camera_system.h"
#include "Engine/engine.h"

VulkanAnimation::VulkanAnimation(VulkanEngine *engine, VulkanUtility *utility) :
	VulkanModule(utility),
	p_vkEngine(engine)
{
}

VulkanAnimation::~VulkanAnimation()
{
}

void VulkanAnimation::Destroy()
{

}

void VulkanAnimation::PrepareMeshDescriptorSet()
{
	std::array<VkDescriptorPoolSize, 1> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_animInfo.mesh.descriptors.pool));

	// scene descriptor layout
	VkDescriptorSetLayoutBinding uboLayout = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);

	VkDescriptorSetLayoutBinding sceneBind[] = { uboLayout };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = sceneBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_animInfo.mesh.descriptors.layout));

	// Create descriptor set for meshes
	VkDescriptorSetLayout layouts[] = { m_animInfo.mesh.descriptors.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_animInfo.mesh.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_animInfo.mesh.descriptors.set));

	VkDescriptorBufferInfo uboBuffInfo = vkUtility->InitBufferInfoDescriptor(m_uboBuffer.buffer, 0, m_uboBuffer.size);

	std::array<VkWriteDescriptorSet, 1> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_animInfo.mesh.descriptors.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanAnimation::PrepareMaterialDescriptorPool(uint32_t materialCount)
{
	// create descriptor pool for all models and materials
	std::array<VkDescriptorPoolSize, 3> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = materialCount;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// diffuse
	descrPoolSize[1].descriptorCount = materialCount;
	descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// normal
	descrPoolSize[2].descriptorCount = materialCount;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = materialCount + 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_animInfo.material.descriptors.pool));
}

void VulkanAnimation::PrepareMaterialDescriptorLayouts()
{
	// create a layout for each possible map arrangment - these will be assigned to models later
	uint32_t bindCount = 0;
	std::vector<VkDescriptorSetLayoutBinding> layoutBind;
	layoutBind.push_back(vkUtility->InitLayoutBinding(bindCount++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));		// diffuse
	
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	layoutInfo.pBindings = layoutBind.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_animInfo.descriptors.diffLayout));

	layoutBind.push_back(vkUtility->InitLayoutBinding(bindCount++, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT));		// diffuse + normal	
	layoutInfo.pBindings = layoutBind.data();
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_animInfo.descriptors.diffnormLayout));
}

void VulkanAnimation::PrepareMaterialDescriptorSets(ColladaModelInfo::Material *material)
{
	std::vector<VkDescriptorSetLayout> layouts;
	std::vector<VkDescriptorImageInfo> imageInfo;
	
	if (material->matTypes & (int)MaterialTexture::TEX_DIFF) {

		layouts.push_back(m_animInfo.descriptors.diffLayout);
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.diffuse.imageView, material->texture.diffuse.m_tex_sampler));		// diffuse texture

		if (material->matTypes & (int)MaterialTexture::TEX_NORM) {

			layouts[0] = (m_animInfo.descriptors.diffnormLayout);
			imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.normal.imageView, material->texture.normal.m_tex_sampler));			// normal texture
		}
	}
	else if (material->matTypes & (int)MaterialTexture::TEX_NONE) {
		
		layouts.push_back(m_animInfo.descriptors.diffLayout);
		imageInfo.push_back(vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, material->texture.nomap.imageView, material->texture.nomap.m_tex_sampler));		// no map - using dummy texture as vulkan requires an imageview
	}

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_animInfo.material.descriptors.pool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
	allocInfo.pSetLayouts = layouts.data();
	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &material->descrSet));
		
	std::vector<VkWriteDescriptorSet> writeDescrSet;
	writeDescrSet.resize(imageInfo.size());
	for (int c = 0; c < imageInfo.size(); ++c) {

		writeDescrSet[c] = vkUtility->InitDescriptorSet(material->descrSet, c, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[c]);
	}

	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanAnimation::PreparePipeline()
{
	// pipeline for materials containing diffuse and normal maps
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID);
	
	// scene graphics pipeline
	ColladaModelInfo::ModelVertex vertex;
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

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility->InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	// colour attachment required for each colour buffer
	std::array<VkPipelineColorBlendAttachmentState, 6> colorAttach = {};
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

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 2;

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(ColladaModelInfo::MaterialProperties);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };
	
	std::vector<VkDescriptorSetLayout> descrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.diffnormLayout };
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = static_cast<uint32_t>(descrLayouts.size());
	pipelineInfo.pSetLayouts = descrLayouts.data();
	pipelineInfo.pPushConstantRanges = pushConstantArray;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_animInfo.matPipelines.diffNorm.layout));

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
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.diffNorm.pipeline));

	createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;											// inform the API that we are using a pipeline derivative
	createInfo.basePipelineHandle = m_animInfo.matPipelines.diffNorm.pipeline;

	// diffuse map only pipeline
	std::vector<VkDescriptorSetLayout> dndescrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.diffLayout };
	pipelineInfo.pSetLayouts = dndescrLayouts.data();
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_animInfo.matPipelines.diff.layout));

	createInfo.layout = m_animInfo.matPipelines.diff.layout;
	m_animInfo.shader[1] = vkUtility->InitShaders("skinning/mesh_DIFF-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.diff.pipeline));

	// no-material map pipeline
	std::vector<VkDescriptorSetLayout> nomapdescrLayouts = { m_animInfo.mesh.descriptors.layout, m_animInfo.descriptors.diffLayout };		// no material map shader has a sampler bound but not used
	pipelineInfo.pSetLayouts = nomapdescrLayouts.data();
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_animInfo.matPipelines.nomap.layout));

	createInfo.layout = m_animInfo.matPipelines.nomap.layout;
	m_animInfo.shader[1] = vkUtility->InitShaders("skinning/mesh_NOMAP-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_animInfo.matPipelines.nomap.pipeline));
}

void VulkanAnimation::GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{

	for (auto& index : p_modelManager->m_updatedAnimMeshInd) {			// the number of models; accessed via indexing

		// bind vertex data at offset into buffer
		VkBuffer vertexBuffers = m_vertexBuffer.buffer;
		VkDeviceSize offset = p_modelManager->m_models[index].vertexBuffer.offset;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffers, &offset);
		
		for (uint32_t i = 0; i < p_modelManager->m_colladaModels[index].m_meshData.size(); i++) {		// the number of meshes within this particular model

			for (uint32_t f = 0; f < p_modelManager->m_colladaModels[index].m_meshData[i].faceInfo.size(); ++f) {		// and the number of faces - with each face having its own material

				auto& model = p_modelManager->m_colladaModels[index];

				uint32_t matIndex = model.m_meshData[i].faceInfo[f].materialIndex;
				std::array<VkDescriptorSet, 2> descrSets = { m_animInfo.mesh.descriptors.set, model.m_materials[matIndex].descrSet };
				vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? model.m_materials[matIndex].pipeline : pipeline);
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? model.m_materials[matIndex].layout : layout, 0, (pipeline == VK_NULL_HANDLE) ? static_cast<uint32_t>(descrSets.size()) : 1, (pipeline == VK_NULL_HANDLE) ? descrSets.data() : &set, 0, NULL);

				// bind index data derived from face indices - draw each face with one draw call as material differ between each and we will be pushing the material data per draw call
				vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer.buffer, /* model.indexBuffer.offset + */ model.m_meshData[i].faceInfo[f].indexBase * sizeof(uint32_t), VK_INDEX_TYPE_UINT32);

				if (pipeline == VK_NULL_HANDLE) {
					vkCmdPushConstants(cmdBuffer, model.m_materials[matIndex].layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ColladaModelInfo::MaterialProperties), &model.m_materials[matIndex].properties);		// push material info per face
				}
				vkCmdDrawIndexed(cmdBuffer, model.m_meshData[i].faceInfo[f].indexCount, 1, 0, 0, 0);
			}
		}
	}
}

void VulkanAnimation::CreateVertexBuffer(uint32_t bufferSize)
{
	m_vertexBuffer.size = bufferSize;
	vkUtility->CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffer.buffer, m_vertexBuffer.memory);
}

void VulkanAnimation::CreateIndexBuffer(uint32_t bufferSize)
{
	m_indexBuffer.size = bufferSize;
	vkUtility->CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indexBuffer.buffer, m_indexBuffer.memory);
}

void VulkanAnimation::PrepareUBOBuffer()
{
	m_uboBuffer.size = sizeof(UboLayout);
	vkUtility->CreateBuffer(m_uboBuffer.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uboBuffer.buffer, m_uboBuffer.memory);
}

void VulkanAnimation::MapDataToBuffers(ColladaModelInfo *model, uint32_t vertexOffset, uint32_t indexOffset)
{
	// map vertex data to meta-buffer
	vkMapMemory(p_vkEngine->m_device.device, m_vertexBuffer.memory, vertexOffset, model->vertexBuffer.size, 0, &model->vertexBuffer.mappedData);
	memcpy(model->vertexBuffer.mappedData, model->vertices.data(), model->vertexBuffer.size);
	vkUnmapMemory(p_vkEngine->m_device.device, m_vertexBuffer.memory);

	// map index buffer
	vkMapMemory(p_vkEngine->m_device.device, m_indexBuffer.memory, indexOffset, model->indexBuffer.size, 0, &model->indexBuffer.mappedData);
	memcpy(model->indexBuffer.mappedData, model->indices.data(), model->indexBuffer.size);
	vkUnmapMemory(p_vkEngine->m_device.device, m_indexBuffer.memory);
}

void VulkanAnimation::Update(CameraSystem *camera)
{
	std::vector<UboLayout> uboData;

	UboLayout ubo;
	ubo.projection = camera->m_cameraInfo.projection;
	ubo.viewMatrix = camera->m_cameraInfo.viewMatrix;

	ubo.modelMatrix = p_modelManager->GetUpdatedTransform(0, ModelType::MODEL_ANIMATED);		// ****** CHANGE!!!! *****

	// update bone animation transforms for each model
	for (auto& model : p_modelManager->m_colladaModels) {
		
		//model.UpdateModelAnimation();

		for (uint32_t c = 0; c < model.m_boneTransforms.size(); ++c) {

			ubo.boneTransform[c] = model.m_boneTransforms[c];
		}
	}
	uboData.push_back(ubo);

	vkUtility->MapBuffer<UboLayout>(m_uboBuffer, uboData);
}