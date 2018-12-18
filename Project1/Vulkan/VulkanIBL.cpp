#include "VulkanIBL.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VkDescriptors.h"
#include "VulkanCore/VulkanRenderPass.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/VulkanModel.h"
#include "Systems/camera_system.h"

VulkanIBL::VulkanIBL(VulkanEngine *engine, VkMemoryManager *memory) :
	VulkanModule(memory),
	p_vkEngine(engine)
{
	Init();
}


VulkanIBL::~VulkanIBL()
{
	Destroy();
}

void VulkanIBL::LoadAssets()
{
	m_cubeImage = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_cubeImage->LoadCubeMap("assets/textures/skybox/hdr_cube.ktx", VK_FORMAT_R16G16B16A16_SFLOAT, p_vkEngine->GetCmdPool(), p_vkEngine->GetGraphQueue(), p_vkMemory);
}

void VulkanIBL::SetupIBL()
{
	// prepare images for cube mapping 
	m_irradianceCube.cubeImage = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_irradianceCube.cubeImage->PrepareImageArray(VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, IRRADIANCEMAP_DIM, IRRADIANCEMAP_DIM, MIP_LEVELS, 6, true);

	m_filterCube.cubeImage = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_filterCube.cubeImage->PrepareImageArray(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, PREFILTERMAP_DIM, PREFILTERMAP_DIM, MIP_LEVELS, 6, true);

	// prepare images for offscreen rendering
	m_irradianceCube.offscreenImage = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_irradianceCube.offscreenImage->PrepareImage(VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, IRRADIANCEMAP_DIM, IRRADIANCEMAP_DIM, 0, false);

	m_filterCube.offscreenImage = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_filterCube.offscreenImage->PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, PREFILTERMAP_DIM, PREFILTERMAP_DIM, 0, false);

	// irradiance cube renderpass and framebuffer
	// create renderpass with colour attachment
	m_irradianceCube.renderpass = new VulkanRenderPass(p_vkEngine->GetDevice());
	m_irradianceCube.renderpass->AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_FORMAT_R32G32B32A32_SFLOAT);
	m_irradianceCube.renderpass->AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
	m_irradianceCube.renderpass->PrepareRenderPass(p_vkEngine->GetDevice());

	m_irradianceCube.renderpass->PrepareFramebuffer(m_irradianceCube.offscreenImage->imageView, IRRADIANCEMAP_DIM, IRRADIANCEMAP_DIM, p_vkEngine->GetDevice());

	// create offscreen frame buffer for pre-filtered cube
	m_filterCube.renderpass = new VulkanRenderPass(p_vkEngine->GetDevice());
	m_filterCube.renderpass->AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_FORMAT_R16G16B16A16_SFLOAT);
	m_filterCube.renderpass->AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
	m_filterCube.renderpass->PrepareRenderPass(p_vkEngine->GetDevice());

	m_filterCube.renderpass->PrepareFramebuffer(m_filterCube.offscreenImage->imageView, PREFILTERMAP_DIM, PREFILTERMAP_DIM, p_vkEngine->GetDevice());

	// transition images to format ready for reading from later
	VkCommandBuffer cmdBuffer = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

	// irradiance transition
	VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_irradianceCube.offscreenImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

	// pre-filter transition
	VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_filterCube.offscreenImage->image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

	VulkanUtility::SubmitCmdBufferToQueue(cmdBuffer, p_vkEngine->GetGraphQueue(), p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());
}

void VulkanIBL::PrepareIBLDescriptors()
{
	m_descriptors = new VkDescriptors(p_vkEngine->GetDevice());

	std::vector<VkDescriptors::LayoutBinding> layoutBind = 
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }			// bindings for the colour image sampler
	};
	m_descriptors->AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorImageInfo> imageInfo = 
	{
		{ m_cubeImage->texSampler, m_cubeImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_descriptors->GenerateDescriptorSets(nullptr, imageInfo.data());
}

void VulkanIBL::PrepareIBLPipeline()
{
	// offscreen pipeline
	VulkanModel::ModelVertex vertex;
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

	VkPipelineRasterizationStateCreateInfo rasterInfo = VulkanUtility::InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = VulkanUtility::InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

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
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(FilterPushConstant);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_descriptors->layout;
	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_pipelineLayout));

	// sahders for rendering full screen quad
	m_shader[0] = VulkanUtility::InitShaders("IBL/cubemap-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_shader[1] = VulkanUtility::InitShaders("IBL/cubemap_irradiance-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());

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
	createInfo.layout = m_pipelineLayout;
	createInfo.renderPass = m_irradianceCube.renderpass->renderpass;
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_irradianceCube.pipeline));

	// pre-filter pipeline
	createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;											
	createInfo.basePipelineHandle = m_irradianceCube.pipeline;
	createInfo.renderPass = m_filterCube.renderpass->renderpass;
	m_shader[1] = VulkanUtility::InitShaders("IBL/cubemap_prefilter-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_filterCube.pipeline));
}

void VulkanIBL::GenerateIrrMapCmdBuffer()
{
	std::array<VkClearValue, 1> clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_irradianceCube.renderpass->renderpass;
	renderPassInfo.framebuffer = m_irradianceCube.renderpass->frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = m_irradianceCube.cubeImage->width;
	renderPassInfo.renderArea.extent.height = m_irradianceCube.cubeImage->height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();
	
	VkCommandBuffer cmdBuffer = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

	VkViewport viewport = VulkanUtility::InitViewPort(m_irradianceCube.cubeImage->width, m_irradianceCube.cubeImage->height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(m_irradianceCube.cubeImage->width, m_irradianceCube.cubeImage->height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// the cube map object data is loaded with all other models - it is assumed that the cubemap will default at index location 0 in the model map - TODO: ref cubemap data location in .json world data when implemented
	auto p_vkModel = p_vkEngine->VkModule<VulkanModel>();
	VulkanModel::ModelInfo model = p_vkModel->RequestModelInfo(0);

	VkDeviceSize offsets[1]{ p_vkModel->GetVertexOffset() };

	VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_irradianceCube.cubeImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice(), MIP_LEVELS, 6);

	for (int m = 0; m < MIP_LEVELS; ++m) {
		
		for (uint32_t c = 0; c < 6; ++c) {

			float dim = static_cast<float>(IRRADIANCEMAP_DIM * std::pow(0.5f, m));
			VkViewport viewport = VulkanUtility::InitViewPort(static_cast<uint32_t>(dim), static_cast<uint32_t>(dim), 0.0f, 1.0f);
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_irradianceCube.pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptors->set, 0, NULL);
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkModel->GetVertexBuffer(), offsets);
			vkCmdBindIndexBuffer(cmdBuffer, p_vkModel->GetIndexBuffer(), p_vkModel->GetIndexOffset(), VK_INDEX_TYPE_UINT32);

			// render each of the cubes faces
			FilterPushConstant push;
			push.mvp = glm::perspective(PI / 2, 1.0f, 0.1f, 512.0f) * cubeView[c];
			vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FilterPushConstant), &push);

			vkCmdDrawIndexed(cmdBuffer, model.meshes[0].indexCount, 1, 0, 0, 0);
			vkCmdEndRenderPass(cmdBuffer);

			// copy each of the cubemap faces 
			VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_irradianceCube.offscreenImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

			VkImageCopy imageCopy = {};
			imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.srcSubresource.baseArrayLayer = 0;
			imageCopy.srcSubresource.layerCount = 1;
			imageCopy.srcSubresource.mipLevel = 0;
			imageCopy.srcOffset = { 0,0,0 };

			imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.dstSubresource.layerCount = 1;
			imageCopy.dstSubresource.mipLevel = m;
			imageCopy.dstSubresource.baseArrayLayer = c;
			imageCopy.dstOffset = { 0,0,0 };
			imageCopy.extent.depth = 1;
			imageCopy.extent.width = static_cast<uint32_t>(dim);
			imageCopy.extent.height = static_cast<uint32_t>(dim);

			vkCmdCopyImage(cmdBuffer, m_irradianceCube.offscreenImage->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_irradianceCube.cubeImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

			VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_irradianceCube.offscreenImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());
		}
	}
	VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_irradianceCube.cubeImage->image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice(), MIP_LEVELS, 6);

	VulkanUtility::SubmitCmdBufferToQueue(cmdBuffer, p_vkEngine->GetGraphQueue(), p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());
}

void VulkanIBL::GeneratePreFilterCmdBuffer()
{
	std::array<VkClearValue, 1> clearValue{ 0.0f, 0.0f, 0.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_filterCube.renderpass->renderpass;
	renderPassInfo.framebuffer = m_filterCube.renderpass->frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = m_filterCube.cubeImage->width;
	renderPassInfo.renderArea.extent.height = m_filterCube.cubeImage->height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();

	VkCommandBuffer cmdBuffer = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

	VkViewport viewport = VulkanUtility::InitViewPort(m_filterCube.cubeImage->width, m_filterCube.cubeImage->height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(m_filterCube.cubeImage->width, m_filterCube.cubeImage->height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	// the cube map object data is loaded with all other models - it is assumed that the cubemap will default at index location 0 in the model map - TODO: ref cubemap data location in .json world data when implemented
	auto p_vkModel = p_vkEngine->VkModule<VulkanModel>();
	VulkanModel::ModelInfo model = p_vkModel->RequestModelInfo(0);

	VkDeviceSize offsets[1]{ p_vkModel->GetVertexOffset() };

	VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_filterCube.cubeImage->image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice(), MIP_LEVELS, 6);

	FilterPushConstant push;
	for (int m = 0; m < MIP_LEVELS; ++m) {

		push.roughness = (float)m / (float)(MIP_LEVELS - 1);

		for (uint32_t c = 0; c < 6; ++c) {

			float dim = static_cast<float>(PREFILTERMAP_DIM * std::pow(0.5f, m));
			VkViewport viewport = VulkanUtility::InitViewPort(static_cast<uint32_t>(dim), static_cast<uint32_t>(dim), 0.0f, 1.0f);

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_filterCube.pipeline);
			vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptors->set, 0, NULL);
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkModel->GetVertexBuffer(), offsets);
			vkCmdBindIndexBuffer(cmdBuffer, p_vkModel->GetIndexBuffer(), p_vkModel->GetIndexOffset(), VK_INDEX_TYPE_UINT32);

			// render each of the cubes faces
			push.mvp = glm::perspective(PI / 2, 1.0f, 0.1f, 512.0f) * cubeView[c];
			push.sampleCount = 1024;
			vkCmdPushConstants(cmdBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FilterPushConstant), &push);

			vkCmdDrawIndexed(cmdBuffer, model.meshes[0].indexCount, 1, 0, 0, 0);
			vkCmdEndRenderPass(cmdBuffer);

			// copy each of the cubemap faces 
			VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_filterCube.offscreenImage->image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());

			VkImageCopy imageCopy = {};
			imageCopy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.srcSubresource.baseArrayLayer = 0;
			imageCopy.srcSubresource.layerCount = 1;
			imageCopy.srcSubresource.mipLevel = 0;
			imageCopy.srcOffset = { 0,0,0 };

			imageCopy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageCopy.dstSubresource.layerCount = 1;
			imageCopy.dstSubresource.mipLevel = m;
			imageCopy.dstSubresource.baseArrayLayer = c;
			imageCopy.dstOffset = { 0,0,0 };
			imageCopy.extent.depth = 1;
			imageCopy.extent.width = static_cast<uint32_t>(dim);
			imageCopy.extent.height = static_cast<uint32_t>(dim);

			vkCmdCopyImage(cmdBuffer, m_filterCube.offscreenImage->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_filterCube.cubeImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);

			VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_filterCube.offscreenImage->image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());
		}
	}
	VulkanUtility::ImageTransition(p_vkEngine->GetGraphQueue(), cmdBuffer, m_filterCube.cubeImage->image, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice(), MIP_LEVELS, 6);

	VulkanUtility::SubmitCmdBufferToQueue(cmdBuffer, p_vkEngine->GetGraphQueue(), p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());
}

void VulkanIBL::Update(int acc_time)
{	
}

void VulkanIBL::Init()
{
	// load the environment cube
	LoadAssets();

	// generate the image, FB and renderpass for the irradiance and pre-filtered calculations
	SetupIBL();

	// prepare descriptors and pipelines for both
	PrepareIBLDescriptors();
	PrepareIBLPipeline();
}

void VulkanIBL::Destroy()
{
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_filterCube.pipeline, nullptr);
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_irradianceCube.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_pipelineLayout, nullptr);

	delete m_descriptors;

	delete m_filterCube.renderpass;
	delete m_filterCube.cubeImage;
	delete m_filterCube.offscreenImage;

	delete m_irradianceCube.renderpass;
	delete m_irradianceCube.cubeImage;
	delete m_irradianceCube.offscreenImage;

	p_vkEngine = nullptr;
}
