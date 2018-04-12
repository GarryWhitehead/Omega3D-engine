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

void VulkanAnimation::PrepareDescriptorSet(TextureInfo& image)
{
	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				
	descrPoolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_descriptors.pool));

	// scene descriptor layout
	VkDescriptorSetLayoutBinding uboLayout = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding samplerLayout = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);					
	VkDescriptorSetLayoutBinding sceneBind[] = { uboLayout, samplerLayout };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = sceneBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_descriptors.layout));

	// Create descriptor set for meshes
	VkDescriptorSetLayout layouts[] = { m_descriptors.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_descriptors.set));

	VkDescriptorBufferInfo uboBuffInfo = vkUtility->InitBufferInfoDescriptor(m_uboBuffer.buffer, 0, m_uboBuffer.size);
	VkDescriptorImageInfo imageInfo = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_GENERAL, image.imageView, image.m_tex_sampler);

	std::array<VkWriteDescriptorSet, 2> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_descriptors.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_descriptors.set, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo);
	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanAnimation::PreparePipeline()
{
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
	std::array<VkPipelineColorBlendAttachmentState, 3> colorAttach = {};
	colorAttach[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[0].blendEnable = VK_FALSE;
	colorAttach[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[1].blendEnable = VK_FALSE;
	colorAttach[2].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[2].blendEnable = VK_FALSE;

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

	std::vector<VkDescriptorSetLayout> descrLayouts = { m_descriptors.layout };
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = static_cast<uint32_t>(descrLayouts.size());
	pipelineInfo.pSetLayouts = descrLayouts.data();
	pipelineInfo.pPushConstantRanges = pushConstantArray;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_pipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = vkUtility->InitShaders("skinning/mesh-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("skinning/mesh-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	createInfo.layout = m_pipeline.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipeline.pipeline));
}

void VulkanAnimation::GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{

	for (auto& index : p_modelManager->m_updatedAnimMeshInd) {			// the number of models; accessed via indexing

		// bind vertex data at offset into buffer
		VkBuffer vertexBuffers = m_vertexBuffer.buffer;
		VkDeviceSize offset = p_modelManager->m_models[index].vertexBuffer.offset;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffers, &offset);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_pipeline.pipeline : pipeline);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? m_pipeline.layout : layout, 0, 1, (pipeline == VK_NULL_HANDLE) ? &m_descriptors.set : &set, 0, NULL);

		auto& model = p_modelManager->m_colladaModels[index];

		for (uint32_t i = 0; i < p_modelManager->m_colladaModels[index].m_meshData.size(); i++) {		// the number of meshes within this particular model

			for (uint32_t f = 0; f < p_modelManager->m_colladaModels[index].m_meshData[i].faces.size(); ++f) {		// number of faces within this mesh

				// bind index data derived from face indices - draw each face with one draw call as material differ between each and we will be pushing the material data per draw call
				vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer.buffer, model.indexBuffer.offset + model.m_meshData[i].faces[f].indexBase * sizeof(uint32_t), VK_INDEX_TYPE_UINT32);
				uint32_t matIndex = model.m_meshData[i].faces[f].materialIndex;
				
				if (pipeline == VK_NULL_HANDLE) {
					vkCmdPushConstants(cmdBuffer, m_pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ColladaModelInfo::MaterialProperties), &model.m_materials[matIndex].properties);		// push material info per face
				}

				vkCmdDrawIndexed(cmdBuffer, model.m_meshData[i].faces[f].indexCount, 1, 0, 0, 0);
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
		
		model.UpdateModelAnimation();

		for (uint32_t c = 0; c < model.m_boneTransforms.size(); ++c) {

			ubo.boneTransform[c] = model.m_boneTransforms[c];
		}
	}
	uboData.push_back(ubo);

	vkUtility->MapBuffer<UboLayout>(m_uboBuffer, uboData);
}