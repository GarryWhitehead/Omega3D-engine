#include "VulkanCore/vulkan_model.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_shadow.h"
#include "objector/MaterialParser.h"
#include "Systems/camera_system.h"
#include "Engine/ModelResourceManager.h"
#include "ComponentManagers/TransformComponentManager.h"
#include <gtc/matrix_transform.hpp>
#include <iostream>

VulkanModel::VulkanModel(VulkanEngine *engine, VulkanUtility *utility) :
	VulkanModule(utility),
	p_vkEngine(engine),
	vk_prepared(false)
{
}

VulkanModel::~VulkanModel()
{
}

void VulkanModel::CreateVertexBuffer(uint32_t bufferSize)
{
	m_vertexBuffer.size = bufferSize;
	vkUtility->CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffer.buffer, m_vertexBuffer.memory);
}

void VulkanModel::CreateIndexBuffer(uint32_t bufferSize)
{
	m_indexBuffer.size = bufferSize;
	vkUtility->CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indexBuffer.buffer, m_indexBuffer.memory);
}

void VulkanModel::MapDataToBuffers(ModelInfo *model, uint32_t vertexOffset, uint32_t indexOffset)
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

void VulkanModel::PrepareMeshDescriptorSet()
{
	std::array<VkDescriptorPoolSize, 1> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_meshDescrInfo.pool));

	// scene descriptor layout
	VkDescriptorSetLayoutBinding uboLayout = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);						
	
	VkDescriptorSetLayoutBinding sceneBind[] = { uboLayout };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = sceneBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_meshDescrInfo.layout));

	// Create descriptor set for meshes
	VkDescriptorSetLayout layouts[] = { m_meshDescrInfo.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_meshDescrInfo.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &m_meshDescrInfo.set));

	VkDescriptorBufferInfo uboBuffInfo = vkUtility->InitBufferInfoDescriptor(m_uboBuffer.buffer, 0, m_uboBuffer.size);											

	std::array<VkWriteDescriptorSet, 1> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_meshDescrInfo.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
}

void VulkanModel::PrespareMaterialDescriptorPool(uint32_t materialCount)
{
	// create descriptor pool for all models and materials
	std::array<VkDescriptorPoolSize, 3> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = materialCount;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// diffuse
	descrPoolSize[1].descriptorCount = materialCount;
	descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// specular
	descrPoolSize[2].descriptorCount = materialCount;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = materialCount + 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->m_device.device, &createInfo, nullptr, &m_materialDescrInfo.pool));
}

void VulkanModel::PrepareMaterialDescriptorSet(ModelInfo *model)
{
	// material descriptor bindings

	VkDescriptorSetLayoutBinding samplerLayoutDiff = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);		// diffuse	- set 1 / loc 0
	VkDescriptorSetLayoutBinding samplerLayoutSpec = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);		// specular	- set 1 / loc 1
																																	
	VkDescriptorSetLayoutBinding materialBind[] = { samplerLayoutDiff, samplerLayoutSpec };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 2;
	layoutInfo.pBindings = materialBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->m_device.device, &layoutInfo, nullptr, &m_materialDescrInfo.layout));

	// Create a decriptor set for each material type
	for (uint32_t c = 0; c < model->materialData.size(); ++c) {

		VkDescriptorSetLayout layouts[] = { m_materialDescrInfo.layout };
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_materialDescrInfo.pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

		VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->m_device.device, &allocInfo, &model->materialData[c].descrSet));

		VkDescriptorImageInfo imageInfoDiff = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, model->materialData[c].diffuse.imageView, model->materialData[c].diffuse.m_tex_sampler);		// diffuse texture
		VkDescriptorImageInfo imageInfoSpec = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, model->materialData[c].specular.imageView, model->materialData[c].specular.m_tex_sampler);		// diffuse texture

		std::array<VkWriteDescriptorSet, 2> writeDescrSet = {};
		writeDescrSet[0] = vkUtility->InitDescriptorSet(model->materialData[c].descrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfoDiff);
		writeDescrSet[1] = vkUtility->InitDescriptorSet(model->materialData[c].descrSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfoSpec);

		vkUpdateDescriptorSets(p_vkEngine->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
	}
}

void VulkanModel::PrepareModelPipelineWithMaterial()
{
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID);

	// scene graphics pipeline
	ModelInfo::ModelVertex vertex;
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

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->m_viewport.viewPort, p_vkEngine->m_viewport.scissor, p_vkEngine->m_surface.extent.width, p_vkEngine->m_surface.extent.height);

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
	pushConstant.size = sizeof(ModelInfo::MaterialProperties);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };

	VkDescriptorSetLayout setLayouts[] = { m_meshDescrInfo.layout, m_materialDescrInfo.layout };
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 2;
	pipelineInfo.pSetLayouts = setLayouts;
	pipelineInfo.pPushConstantRanges = pushConstantArray;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_matPipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = vkUtility->InitShaders("model/model-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("model/model-mat-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	createInfo.layout = m_matPipeline.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();		// rendered into the offcsreen buffer 
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_matPipeline.pipeline));
}

void VulkanModel::PrepareModelPipelineWithoutMaterial()
{
	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID);

	// scene graphics pipeline
	ModelInfo::ModelVertex vertex;
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

	std::vector<VkDescriptorSetLayout> descrLayouts = { m_meshDescrInfo.layout };
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = static_cast<uint32_t>(descrLayouts.size());
	pipelineInfo.pSetLayouts = descrLayouts.data();
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->m_device.device, &pipelineInfo, nullptr, &m_noMatPipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = vkUtility->InitShaders("model/model-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("model/model-nomat-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	createInfo.layout = m_noMatPipeline.layout;
	createInfo.renderPass = vkDeferred->GetRenderPass();
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_noMatPipeline.pipeline));
}

void VulkanModel::GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	
	for (auto& index : p_modelManager->m_updatedStaticMeshInd) {

		// bind vertex data at offset into buffer
		VkBuffer vertexBuffers = m_vertexBuffer.buffer;
		VkDeviceSize offset = p_modelManager->m_models[index].vertexBuffer.offset;
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffers, &offset);

		for (uint32_t i = 0; i < p_modelManager->m_models[index].meshData.size(); i++) {

			auto& model = p_modelManager->m_models[index];
			
			// bind index data derived from face indices
			vkCmdBindIndexBuffer(cmdBuffer, m_indexBuffer.buffer, model.indexBuffer.offset + model.meshData[i].indexBase * sizeof(uint32_t), VK_INDEX_TYPE_UINT32);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, (pipeline == VK_NULL_HANDLE) ? model.meshData[i].pipeline : pipeline);

			// if alternate pipeline being used, bind their descriptor set
			if (pipeline != VK_NULL_HANDLE) {
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &set, 0, NULL);
			}
			else {
				vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, model.meshData[i].pipelineLayout, 0, model.meshData[i].descrSets.size(), model.meshData[i].descrSets.data(), 0, NULL);

				if (!model.materialData.empty()) {

					// meshes which are associated with material also require data pushing to the shader
					vkCmdPushConstants(cmdBuffer, m_matPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ModelInfo::MaterialProperties), &model.meshData[i].material->properties);
				}
			}
			vkCmdDrawIndexed(cmdBuffer, model.meshData[i].indexCount, 1, 0, 0, 0);
		}
	}
}

void VulkanModel::PrepareUBOBuffer()
{
	m_uboBuffer.size = sizeof(UboLayout);
	vkUtility->CreateBuffer(m_uboBuffer.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uboBuffer.buffer, m_uboBuffer.memory);
}

void VulkanModel::Update(CameraSystem *camera)
{
	std::vector<UboLayout> uboData;

	UboLayout ubo;

	ubo.projection = camera->m_cameraInfo.projection;
	ubo.viewMatrix = camera->m_cameraInfo.viewMatrix;
	ubo.modelMatrix = p_modelManager->GetUpdatedTransform(0, ModelType::MODEL_STATIC);		// ****** CHANGE!!!! *****
	uboData.push_back(ubo);

	vkUtility->MapBuffer<UboLayout>(m_uboBuffer, uboData);
}

void VulkanModel::Destroy()
{

}


