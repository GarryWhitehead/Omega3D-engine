#include "VulkanCore/vulkan_model.h"
#include "VulkanCore/vulkan_render.h"
#include "VulkanCore/vulkan_scene.h"
#include "objector/MaterialParser.h"
#include "Systems/camera_system.h"
#include <gtc/matrix_transform.hpp>
#include <iostream>

VulkanModel::VulkanModel() : 
	vk_prepared(false)
{
}

VulkanModel::VulkanModel(VulkanScene *vulkanScene) :
	VulkanUtility(vulkanScene),
	p_vulkanScene(vulkanScene),
	vk_prepared(false)
{

}

VulkanModel::~VulkanModel()
{
}

void VulkanModel::LoadScene(std::string filename)
{
	m_model = objector.ImportObjFile(filename);

	if (!objector.isObjImported()) {
		std::cout << "Error reading file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}
	else {
		
		if (m_model->numMaterials > 0) {

			this->LoadMaterials();
		}
		
		this->LoadMeshes();
	}
}

void VulkanModel::LoadMeshes()
{
	
	m_meshData.resize(m_model->numMeshes);
	int32_t base = 0;

	for (int c = 0; c < m_meshData.size(); ++c) {

		Objector::objMesh mesh = m_model->meshData[c];

		if (m_model->numMaterials > 0) {
			m_meshData[c].material = &m_materialData[mesh.materialIndex];
		}

		m_meshData[c].indexBase = base;
		m_meshData[c].indexCount = mesh.numFaces() * mesh.faceData[0].numIndices();

		// upload vertices
		for (uint32_t i = 0; i < mesh.numPositions(); ++i) {
			Vertex vertex;
			
			if (mesh.hasPositions()) {
				vertex.pos = mesh.posData[i];
				vertex.pos.y = -vertex.pos.y;
			}
			else {
				vertex.pos = glm::vec3(0.0f);
			}

			if(mesh.hasUv()) {
				vertex.uv = mesh.uvData[i];
			}
			else {
				vertex.uv = glm::vec2(0.0f);
			}

			if (mesh.hasNormals()) {
				vertex.normal = mesh.normData[i];
				vertex.normal.y = -vertex.normal.y;						// flip for vulkan
			}
			else {
				vertex.normal = glm::vec3(0.0f);
			}

			if (mesh.hasColors()) {
				vertex.colour = mesh.colorData[i];
			}
			else {
				vertex.colour = glm::vec3(1.0f, 1.0f, 1.0f);
			}

			m_vertices.push_back(vertex);
		}

		// retrieve the indices data
		for (uint32_t i = 0; i < mesh.numFaces(); ++i) {

			for (uint32_t v = 0; v < mesh.faceData[i].numIndices(); ++v) {

				m_indices.push_back(mesh.faceData[i].indices[v]);
			}
		}

		base += mesh.numFaces() * 3;
	}

	// Map vertex and index buffers to memory
	// TODO: map to local device memory for faster access
	this->MapVertexBufferToMemory();
	this->MapIndexBufferToMemory();
}

void VulkanModel::LoadMaterials()
{

	m_materialData.resize(m_model->numMaterials);

	for (int c = 0; c < m_materialData.size(); ++c) {
		
		m_materialData[c] = {};

		objMaterial material = m_model->materials[c];

		// get material name
		m_materialData[c].name = material.name;

		// Get the material properties
		m_materialData[c].properties.ambient = glm::vec4(material.Color.ambient, 0.0f);
		m_materialData[c].properties.diffuse = glm::vec4(material.Color.diffuse, 0.0f);
		m_materialData[c].properties.specular = glm::vec4(material.Color.specular, 0.0f);
		m_materialData[c].properties.opacity = material.opacity;

		// load material textures and create texture in memory
		m_materialData[c].diffuse = LoadMaterialTexture(material, objTextureType::DIFFUSE_TEXTURE);
		m_materialData[c].specular = LoadMaterialTexture(material, objTextureType::SPECULAR_TEXTURE);
	}
}

TextureInfo VulkanModel::LoadMaterialTexture(objMaterial &material, objTextureType type)
{
	if (!material.hasTextureType(type)) {
		
	}

	std::string filename = material.GetTextureTypeFilename(type);

	filename = filename.substr(0, filename.find(".png"));
	filename = filename + ".ktx";

	TextureInfo texture = LoadTexture(filename, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_COMPARE_OP_ALWAYS, 16, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R8G8B8A8_UNORM, p_vulkanScene->m_cmdPool);
	
	return texture;
}

void VulkanModel::PrepareModelDescriptorSets()
{
	std::vector<VkDescriptorPoolSize> descrPoolSize = {};

	if (m_model->numMaterials > 0) {
		// texture samplers required for materials
		
		descrPoolSize.resize(3);
		descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descrPoolSize[0].descriptorCount = m_materialData.size();
		descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// diffuse
		descrPoolSize[1].descriptorCount = m_materialData.size();
		descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;				// specular
		descrPoolSize[2].descriptorCount = m_materialData.size();
	}
	else {
		descrPoolSize.resize(1);
		descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descrPoolSize[0].descriptorCount = 1;
	}

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = static_cast<uint32_t>(m_materialData.size()) + 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vulkanScene->m_device.device, &createInfo, nullptr, &m_descrPool));

	// scene descriptor layout
	VkDescriptorSetLayoutBinding uboLayout = InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);						// vertex data
	VkDescriptorSetLayoutBinding sceneBind[] = { uboLayout };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;																																
	layoutInfo.pBindings = sceneBind;

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vulkanScene->m_device.device, &layoutInfo, nullptr, &m_sceneDescr.layout));

	// Create descriptor set for the scene data
	VkDescriptorSetLayout layouts[] = { m_sceneDescr.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descrPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vulkanScene->m_device.device, &allocInfo, &m_sceneDescrSet));

	VkDescriptorBufferInfo uboBuffInfo = InitBufferInfoDescriptor(m_uboBuffer.buffer, 0, m_uboBuffer.size);											// Buffer information - UBO buffer size and location

	VkWriteDescriptorSet writeDescrSet = InitDescriptorSet(m_sceneDescrSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBuffInfo);
	vkUpdateDescriptorSets(p_vulkanScene->m_device.device, 1, &writeDescrSet, 0, nullptr);

	// check whether this model has material data
	if (m_model->numMaterials > 0) {

		VkDescriptorSetLayoutBinding samplerLayoutDiff = InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);		// diffuse	- set 1 / loc 0
		VkDescriptorSetLayoutBinding samplerLayoutSpec = InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);		// specular	- set 1 / loc 1

		VkDescriptorSetLayoutBinding materialBind[] = { samplerLayoutDiff, samplerLayoutSpec };
		layoutInfo.bindingCount = 2;
		layoutInfo.pBindings = materialBind;

		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vulkanScene->m_device.device, &layoutInfo, nullptr, &m_materialDescr.layout));

		// Create a decriptor set for each material type
		for (int c = 0; c < m_materialData.size(); ++c) {

			VkDescriptorSetLayout layouts[] = { m_materialDescr.layout };
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = m_descrPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = layouts;

			VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vulkanScene->m_device.device, &allocInfo, &m_materialData[c].descrSet));

			VkDescriptorImageInfo imageInfoDiff = InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_materialData[c].diffuse.imageView, m_materialData[c].diffuse.m_tex_sampler);		// diffuse texture
			VkDescriptorImageInfo imageInfoSpec = InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_materialData[c].specular.imageView, m_materialData[c].specular.m_tex_sampler);		// diffuse texture

			std::array<VkWriteDescriptorSet, 2> writeDescrSet = {};
			writeDescrSet[0] = InitDescriptorSet(m_materialData[c].descrSet, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfoDiff);
			writeDescrSet[1] = InitDescriptorSet(m_materialData[c].descrSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfoSpec);

			vkUpdateDescriptorSets(p_vulkanScene->m_device.device, static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
		}
	}
}

void VulkanModel::PrepareModelPipelineWithMaterial()
{
	// scene graphics pipeline
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

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 2;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_FALSE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(MaterialProperties);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };

	VkDescriptorSetLayout setLayouts[] = { m_sceneDescr.layout, m_materialDescr.layout };
	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 2;
	pipelineInfo.pSetLayouts = setLayouts;
	pipelineInfo.pPushConstantRanges = pushConstantArray;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vulkanScene->m_device.device, &pipelineInfo, nullptr, &m_matPipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = InitShaders("model-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = InitShaders("model-mat-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	createInfo.pDynamicState = nullptr;
	createInfo.layout = m_matPipeline.layout;
	createInfo.renderPass = p_vulkanScene->m_renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	//createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vulkanScene->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_matPipeline.pipeline));
}

void VulkanModel::PrepareModelPipelineWithoutMaterial()
{
	// scene graphics pipeline
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

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_sceneDescr.layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vulkanScene->m_device.device, &pipelineInfo, nullptr, &m_noMatPipeline.layout));

	// load the shaders with tyexture samplers for material textures
	m_shader[0] = InitShaders("model-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = InitShaders("model-nomat-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	createInfo.pDynamicState = nullptr;
	createInfo.layout = m_noMatPipeline.layout;
	createInfo.renderPass = p_vulkanScene->m_renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	//createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vulkanScene->m_device.device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_noMatPipeline.pipeline));
}

void VulkanModel::GenerateModelCmdBuffer()
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

		// bind vertex data
		VkDeviceSize bgOffsets[] = { 0 };
		VkBuffer bgVertexBuffers[] = { m_vertexBuffer.buffer };
		vkCmdBindVertexBuffers(m_cmdBuffer[c], 0, 1, bgVertexBuffers, bgOffsets);

		if (m_model->numMaterials > 0) {

			for (uint32_t i = 0; i < m_model->numMeshes; i++) {

				// bind index data derived from face indices
				vkCmdBindIndexBuffer(m_cmdBuffer[c], m_indexBuffer.buffer, m_meshData[i].indexBase * sizeof(uint32_t), VK_INDEX_TYPE_UINT32);

				std::array<VkDescriptorSet, 2> descrSets = { m_sceneDescrSet, m_meshData[i].material->descrSet };

				vkCmdBindPipeline(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_matPipeline.pipeline);
				vkCmdBindDescriptorSets(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_matPipeline.layout, 0, static_cast<uint32_t>(descrSets.size()), descrSets.data(), 0, NULL);

				// push material data to fragment shader
				vkCmdPushConstants(m_cmdBuffer[c], m_matPipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MaterialProperties), &m_meshData[i].material->properties);

				vkCmdDrawIndexed(m_cmdBuffer[c], m_meshData[i].indexCount, 1, 0, 0, 0);
			}
		}
		else {

			// bind index data derived from face indices
			vkCmdBindIndexBuffer(m_cmdBuffer[c], m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindPipeline(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_noMatPipeline.pipeline);
			vkCmdBindDescriptorSets(m_cmdBuffer[c], VK_PIPELINE_BIND_POINT_GRAPHICS, m_noMatPipeline.layout, 0, 1, &m_sceneDescrSet, 0, NULL);

			for (uint32_t i = 0; i < m_model->numMeshes; ++i) {

				vkCmdDrawIndexed(m_cmdBuffer[c], m_meshData[i].indexCount, 1, 0, m_meshData[i].indexBase, 0);
			}
		}

		// end of the command definitions for this render pass so tell the GPU
		vkCmdEndRenderPass(m_cmdBuffer[c]);
		VK_CHECK_RESULT(vkEndCommandBuffer(m_cmdBuffer[c]));
	}
}

void VulkanModel::MapVertexBufferToMemory()
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	m_vertexBuffer.size = sizeof(Vertex) * m_vertices.size();

	CreateBuffer(m_vertexBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

	vkMapMemory(p_vulkanScene->m_device.device, stagingMemory, 0, m_vertexBuffer.size, 0, &m_vertexBuffer.mappedData);
	memcpy(m_vertexBuffer.mappedData, m_vertices.data(), m_vertexBuffer.size);
	vkUnmapMemory(p_vulkanScene->m_device.device, stagingMemory);

	CreateBuffer(m_vertexBuffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer.buffer, m_vertexBuffer.memory);

	CopyBuffer(stagingBuffer, m_vertexBuffer.buffer, m_vertexBuffer.size, p_vulkanScene->m_cmdPool);

	vkDestroyBuffer(p_vulkanScene->m_device.device, stagingBuffer, nullptr);
	vkFreeMemory(p_vulkanScene->m_device.device, stagingMemory, nullptr);
}

void VulkanModel::MapIndexBufferToMemory()
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;

	m_indexBuffer.size = sizeof(uint32_t) * m_indices.size();

	CreateBuffer(m_indexBuffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingMemory);

	vkMapMemory(p_vulkanScene->m_device.device, stagingMemory, 0, m_indexBuffer.size, 0, &m_indexBuffer.mappedData);
	memcpy(m_indexBuffer.mappedData, m_indices.data(), m_indexBuffer.size);
	vkUnmapMemory(p_vulkanScene->m_device.device, stagingMemory);

	CreateBuffer(m_indexBuffer.size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer.buffer, m_indexBuffer.memory);

	CopyBuffer(stagingBuffer, m_indexBuffer.buffer, m_indexBuffer.size, p_vulkanScene->m_cmdPool);

	vkDestroyBuffer(p_vulkanScene->m_device.device, stagingBuffer, nullptr);
	vkFreeMemory(p_vulkanScene->m_device.device, stagingMemory, nullptr);
}

void VulkanModel::PrepareUBOBuffer()
{
	m_uboBuffer.size = sizeof(UboLayout);
	CreateBuffer(m_uboBuffer.size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uboBuffer.buffer, m_uboBuffer.memory);
}

void VulkanModel::Update(CameraSystem *camera)
{
	std::vector<UboLayout> uboData;

	UboLayout ubo;
	ubo.lightPos = glm::vec4(1.25f, 8.35f, 0.0f, 0.0f);
	ubo.projection = camera->m_cameraInfo.projection;
	ubo.viewMatrix = camera->m_cameraInfo.viewMatrix;
	ubo.modelMatrix = glm::mat4(1.0f);
	uboData.push_back(ubo);

	MapBuffer<UboLayout>(m_uboBuffer, uboData);
}

void VulkanModel::InitVulkanModel()
{
	// load model data - loads materials, then the vertex information, creates the device memory buffers and uploads the data
	this->LoadScene("assets/giraffe.obj");

	this->PrepareUBOBuffer();

	this->PrepareModelDescriptorSets();

	if (m_model->numMaterials > 0) {

		this->PrepareModelPipelineWithMaterial();
	}
	else {
		this->PrepareModelPipelineWithoutMaterial();
	}

	this->GenerateModelCmdBuffer();

	vk_prepared = true;
}

VkVertexInputBindingDescription VulkanModel::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 4> VulkanModel::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: uv
	std::array<VkVertexInputAttributeDescription, 4> attrDescr = {};
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

	// Vertex layout 3: view vector
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(Vertex, colour);


	return attrDescr;
}