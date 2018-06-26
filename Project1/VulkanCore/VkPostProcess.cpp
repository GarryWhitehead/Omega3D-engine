#include "VkPostProcess.h"
#include "VulkanCore/VkDescriptors.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VulkanRenderPass.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanDeferred.h"
#include "Systems/camera_system.h"
#include "Engine/World.h"
#include <gtc/matrix_transform.hpp>

VkPostProcess::VkPostProcess(VulkanEngine *engine, VkMemoryManager *memory) :
	VulkanModule(memory),
	p_vkEngine(engine),
	offscreen_cmdBuffer(VK_NULL_HANDLE)
{
	Init();
}

VkPostProcess::~VkPostProcess()
{
}

void VkPostProcess::PrepareBuffers()
{
	m_finalInfo.uboBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(PostProcessVS));
	
	m_vertBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(FogUboFS));

	// buffer for the blur shader
	m_bloomInfo.uboBuffer = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(PostProcessVS));

	// we can initialise the bloom ubo now as this will remain static
	std::vector<BlurUbo> ubo(1);
	ubo[0].blurScale = BLUR_SCALE;
	ubo[0].blurStrength = BLUR_STRENGTH;

	p_vkMemory->MapDataToSegment<BlurUbo>(m_bloomInfo.uboBuffer, ubo);
}

void VkPostProcess::PrepareFrameBuffers()
{
	uint32_t width = p_vkEngine->GetSurfaceExtentW(); 
	uint32_t height = p_vkEngine->GetSurfaceExtentH();
	
	// create a seperate renderpass with two colour attachments - one for the normal colour scene and one for the bright/bloom scene
	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormat depthFormat = VulkanUtility::FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, p_vkEngine->GetPhysicalDevice());

	// prepare two FBs - one for normal scene and one for bloom rendering - with gaussian blur
	// first the texture sampler images for normal scene
	m_images.normalCol = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_images.normalCol->PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height, 16.0f, true, 1, VK_FILTER_NEAREST);

	// and bright bloom scene
	m_images.brightCol = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_images.brightCol->PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height, 16.0f);

	// depth for colour pass
	m_images.depth = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_images.depth->PrepareImage(depthFormat, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height);

	// gussian blur image 
	m_images.bloom = new VulkanTexture(p_vkEngine->GetPhysicalDevice(), p_vkEngine->GetDevice());
	m_images.bloom->PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, width, height, 1.0f, true, 1, VK_FILTER_NEAREST);
	
	// create a seperate renderpass with two colour attachments - one for the normal colour scene and one for the bright/bloom scene
	m_ppInfo.renderpass = new VulkanRenderPass(p_vkEngine->GetDevice());
	m_ppInfo.renderpass->AddAttachment(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_FORMAT_R16G16B16A16_SFLOAT);
	m_ppInfo.renderpass->AddAttachment(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_FORMAT_R16G16B16A16_SFLOAT);
	m_ppInfo.renderpass->AddAttachment(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthFormat);
	m_ppInfo.renderpass->AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);		// normal
	m_ppInfo.renderpass->AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);		// bloom
	m_ppInfo.renderpass->AddReference(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 2);
	m_ppInfo.renderpass->PrepareRenderPass(p_vkEngine->GetDevice());

	// and the FB with the two buffer attachmentsa
	std::vector<VkFramebuffer> frameBuffers
	{
		m_images.normalCol->imageView,
		m_images.brightCol->imageView,
		m_images.depth->imageView
	};

	m_ppInfo.renderpass->PrepareFramebuffer(frameBuffers, width, height, p_vkEngine->GetDevice());

	// create renderpass and FB for bloom blur
	m_bloomInfo.renderpassHoriz = new VulkanRenderPass(p_vkEngine->GetDevice());
	m_bloomInfo.renderpassHoriz->AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_FORMAT_R16G16B16A16_SFLOAT);
	m_bloomInfo.renderpassHoriz->AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);		
	m_bloomInfo.renderpassHoriz->PrepareRenderPass(p_vkEngine->GetDevice());

	m_bloomInfo.renderpassVert = new VulkanRenderPass(p_vkEngine->GetDevice(), m_bloomInfo.renderpassHoriz->renderpass);		// use the same renderpass as the horizontal, only the FB differs

	// frame buffer for vertical and horizontal passes
	m_bloomInfo.renderpassHoriz->PrepareFramebuffer(m_images.bloom->imageView, width, height, p_vkEngine->GetDevice());					// horizontal frame buffer
	m_bloomInfo.renderpassVert->PrepareFramebuffer(m_images.bloom->imageView, width, height, p_vkEngine->GetDevice());		// vertical frame buffer
}

void VkPostProcess::GenerateColPassCmdBuffer(VkCommandBuffer cmdBuffer)
{
	// using sperate cmd buffers for each pass and semaphores as the colour pass needs to complete before the blur pass.
	// and then onces this is complete, the final compisition can be generated along with tone mapping and fog
	std::array<VkClearValue, 3> clearValue;
	clearValue[0] = { VulkanEngine::CLEAR_COLOR };
	clearValue[1] = { VulkanEngine::CLEAR_COLOR };
	clearValue[2].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	renderPassInfo.renderPass = m_ppInfo.renderpass->renderpass;
	renderPassInfo.framebuffer = m_ppInfo.renderpass->frameBuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = p_vkEngine->GetSurfaceExtentW();
	renderPassInfo.renderArea.extent.height = p_vkEngine->GetSurfaceExtentH();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();

	VkViewport viewport = VulkanUtility::InitViewPort(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	VkDeviceSize offsets[1]{ m_vertices.offset };
	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppInfo.pipelineInfo.pipeline);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertices.block_id), offsets);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_ppInfo.pipelineInfo.layout, 0, 1, &m_ppInfo.descriptors->set, 0, NULL);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indices.block_id), m_indices.offset, VK_INDEX_TYPE_UINT32);

	// draw as a full-screen quad as our models, terrain, etc. have already been generated
	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);
}

void VkPostProcess::GenerateBlurPassCmdBuffer(VkCommandBuffer& cmdBuffer, VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t width, uint32_t height, uint32_t direction)
{	
	std::array<VkClearValue, 2> clearValue;
	clearValue[0] = { VulkanEngine::CLEAR_COLOR };
	clearValue[1] = { VulkanEngine::CLEAR_COLOR };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;

	// first pass - horizontal blur
	renderPassInfo.renderPass = renderpass;
	renderPassInfo.framebuffer = framebuffer;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = width;
	renderPassInfo.renderArea.extent.height = height;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValue.size());
	renderPassInfo.pClearValues = clearValue.data();

	VkViewport viewport = VulkanUtility::InitViewPort(width, height, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(width, height, 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	VkDeviceSize offsets[1]{ m_vertices.offset };
	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomInfo.pipelineInfo.pipeline);
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertices.block_id), offsets);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_bloomInfo.pipelineInfo.layout, 0, 1, &m_bloomInfo.descriptors->set, 0, NULL);

	// send horizontal key to shader
	uint32_t push = direction;
	vkCmdPushConstants(cmdBuffer, m_bloomInfo.pipelineInfo.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &push);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indices.block_id), m_indices.offset, VK_INDEX_TYPE_UINT32);

	// draw as a full-screen quad as our models, terrain, etc. have already been generated
	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);

	vkCmdEndRenderPass(cmdBuffer);
	
}

void VkPostProcess::GenerateFinalCmdBuffer(VkCommandBuffer cmdBuffer)
{
	VkViewport viewport = VulkanUtility::InitViewPort(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = VulkanUtility::InitScissor(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0, 0);
	vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

	VkDeviceSize offsets[1]{ m_vertices.offset };

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_finalInfo.pipelineInfo.pipeline);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertices.block_id), offsets);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_finalInfo.pipelineInfo.layout, 0, 1, &m_finalInfo.descriptors->set, 0, NULL);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indices.block_id), m_indices.offset, VK_INDEX_TYPE_UINT32);

	// draw as a full-screen quad
	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
}

void VkPostProcess::PrepareFullscreenQuad()
{
	// prepare vertices
	std::vector<Vertex> vertices;
	vertices.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f }, } );
	vertices.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f }, } );
	vertices.push_back({ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f }, } );
	vertices.push_back({ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f }, } );

	// prepare indices
	std::vector<uint32_t> indices = { 0,1,2, 2,3,0 };
	for (uint32_t i = 0; i < 3; ++i)
	{
		uint32_t values[6] = { 0,1,2, 2,3,0 };
		for (auto index : values)
		{
			indices.push_back(i * 4 + index);
		}
	}

	// map vertices
	m_vertices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(Vertex) * vertices.size());
	p_vkMemory->MapDataToSegment<Vertex>(m_vertices, vertices);

	// map indices
	m_indices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * indices.size());
	p_vkMemory->MapDataToSegment<uint32_t>(m_indices, indices);
}

void VkPostProcess::PrepareColourPassDescriptors()
{
	m_ppInfo.descriptors = new VkDescriptors(p_vkEngine->GetDevice());

	auto vkDeferred = p_vkEngine->VkModule<VulkanDeferred>();

	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }					// bindings for the offscreen colour image sampler 
	};

	m_ppInfo.descriptors->AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorBufferInfo> buffInfo =
	{
		{ p_vkMemory->blockBuffer(m_vertBuffer.block_id), m_vertBuffer.offset, m_vertBuffer.size },
	};

	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ vkDeferred->GetOffscreenSampler(), vkDeferred->GetOffscreenImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_ppInfo.descriptors->GenerateDescriptorSets(buffInfo.data(), imageInfo.data());
}

void VkPostProcess::PrepareBlurPassDescriptors()
{
	m_bloomInfo.descriptors = new VkDescriptors(p_vkEngine->GetDevice());

	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }					// bright colour scene
	};

	m_bloomInfo.descriptors->AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorBufferInfo> buffInfo =
	{
		{ p_vkMemory->blockBuffer(m_vertBuffer.block_id), m_vertBuffer.offset, m_vertBuffer.size },
		{ p_vkMemory->blockBuffer(m_bloomInfo.uboBuffer.block_id), m_bloomInfo.uboBuffer.offset, m_bloomInfo.uboBuffer.size }
	};

	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ m_images.brightCol->texSampler, m_images.brightCol->imageView,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_bloomInfo.descriptors->GenerateDescriptorSets(buffInfo.data(), imageInfo.data());
}

void VkPostProcess::PrepareFinalDescriptors()
{
	m_finalInfo.descriptors = new VkDescriptors(p_vkEngine->GetDevice());
	
	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },						// bindings for the UBO	
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },				// bindings for the normal colour scene
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }					// and the blurred bloom scene
	};

	m_finalInfo.descriptors->AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorBufferInfo> buffInfo =
	{
		{ p_vkMemory->blockBuffer(m_vertBuffer.block_id), m_vertBuffer.offset, m_vertBuffer.size },
		{ p_vkMemory->blockBuffer(m_finalInfo.uboBuffer.block_id), m_finalInfo.uboBuffer.offset, m_finalInfo.uboBuffer.size }
	};

	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ m_images.normalCol->texSampler, m_images.normalCol->imageView,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ m_images.bloom->texSampler, m_images.bloom->imageView,  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_finalInfo.descriptors->GenerateDescriptorSets(buffInfo.data(), imageInfo.data());
}

void VkPostProcess::PreparePipelines()
{
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

	VkPipelineViewportStateCreateInfo viewportState = VulkanUtility::InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = VulkanUtility::InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = VulkanUtility::InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	// colour attachment required for each colour buffer
	std::array<VkPipelineColorBlendAttachmentState, 1> colorAttach = {};
	colorAttach[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// additive blending for blurring
	colorAttach[0].blendEnable = VK_TRUE;
	colorAttach[0].colorBlendOp = VK_BLEND_OP_ADD;
	colorAttach[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorAttach[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorAttach[0].alphaBlendOp = VK_BLEND_OP_ADD;
	colorAttach[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorAttach[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;

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

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_finalInfo.descriptors->layout;
	pipelineInfo.pPushConstantRanges = 0;
	pipelineInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_finalInfo.pipelineInfo.layout));

	// sahders for rendering full screen quad - final pipeline
	m_finalInfo.shader[0] = VulkanUtility::InitShaders("Post-Process/postprocess-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());		// all post process shders use the same vert - we are just rendering a full screen quad each time
	m_finalInfo.shader[1] = VulkanUtility::InitShaders("Post-Process/composition-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_finalInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_finalInfo.pipelineInfo.layout;
	createInfo.renderPass = p_vkEngine->GetFinalRenderPass();
	createInfo.subpass = 0;						
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_finalInfo.pipelineInfo.pipeline));

	// bloom - guassian blur pipeline
	m_bloomInfo.shader[0] = VulkanUtility::InitShaders("Post-Process/postprocess-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_bloomInfo.shader[1] = VulkanUtility::InitShaders("Post-Process/bloom-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());
	createInfo.pStages = m_bloomInfo.shader.data();

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(uint32_t);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;
	pipelineInfo.pSetLayouts = &m_bloomInfo.descriptors->layout;
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_bloomInfo.pipelineInfo.layout));

	createInfo.layout = m_bloomInfo.pipelineInfo.layout;
	createInfo.renderPass = m_bloomInfo.renderpassHoriz->renderpass;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_bloomInfo.pipelineInfo.pipeline));

	// pipeline to scene split into normal and bright componenets
	pipelineInfo.pSetLayouts = &m_ppInfo.descriptors->layout;
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_ppInfo.pipelineInfo.layout));

	m_ppInfo.shader[0] = VulkanUtility::InitShaders("Post-Process/postprocess-vert.spv", VK_SHADER_STAGE_VERTEX_BIT, p_vkEngine->GetDevice());
	m_ppInfo.shader[1] = VulkanUtility::InitShaders("Post-Process/colour_pass-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT, p_vkEngine->GetDevice());
	createInfo.pStages = m_ppInfo.shader.data();

	// two colour attachments 
	std::array<VkPipelineColorBlendAttachmentState, 2> colorAttachPP = {};
	colorAttachPP[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttachPP[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttachPP[0].blendEnable = VK_FALSE;
	colorAttachPP[1].blendEnable = VK_FALSE;
	colorInfo.attachmentCount = static_cast<uint32_t>(colorAttachPP.size());
	colorInfo.pAttachments = colorAttachPP.data();

	depthInfo.depthTestEnable = VK_TRUE;
	depthInfo.depthWriteEnable = VK_TRUE;
	rasterInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

	createInfo.layout = m_ppInfo.pipelineInfo.layout;
	createInfo.renderPass = m_ppInfo.renderpass->renderpass;
	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_ppInfo.pipelineInfo.pipeline));
}

void VkPostProcess::DrawBloom()
{
	if (offscreen_cmdBuffer != VK_NULL_HANDLE) {
		vkFreeCommandBuffers(p_vkEngine->GetDevice(), p_vkEngine->GetCmdPool(), 1, &offscreen_cmdBuffer);
	}

	offscreen_cmdBuffer = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetCmdPool(), p_vkEngine->GetDevice());
	
	// process hdr image into normal and bright fbs
	GenerateColPassCmdBuffer(offscreen_cmdBuffer);

	for (uint32_t c = 0; c < 5; ++c) {
		// horiszontal blur
		GenerateBlurPassCmdBuffer(offscreen_cmdBuffer, m_bloomInfo.renderpassVert->renderpass, m_bloomInfo.renderpassVert->frameBuffer, p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0);

		// vertical blur
		GenerateBlurPassCmdBuffer(offscreen_cmdBuffer, m_bloomInfo.renderpassHoriz->renderpass, m_bloomInfo.renderpassHoriz->frameBuffer, p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 1);
	}
	VK_CHECK_RESULT(vkEndCommandBuffer(offscreen_cmdBuffer));
}

void VkPostProcess::Submit(VkSemaphore *last_semaphore)
{
	VkSubmitInfo submit_info = {};
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.pWaitDstStageMask = &flags;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.signalSemaphoreCount = 1;
	submit_info.commandBufferCount = 1;

	// draw normal and bright scene into FB, blur bright scene
	submit_info.pWaitSemaphores = last_semaphore;
	submit_info.pSignalSemaphores = &offscreen_semaphore;
	submit_info.pCommandBuffers = &offscreen_cmdBuffer;
	VK_CHECK_RESULT(vkQueueSubmit(p_vkEngine->GetGraphQueue(), 1, &submit_info, VK_NULL_HANDLE));

	// final stage is the full scene composition render which is drawn to the presentation image
}

void VkPostProcess::Update(int acc_time)
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// upload final composition pass fragment ubo data
	std::vector<FogUboFS> uboFS(1);
	uboFS[0].fogDensity = FOG_DENSITY;
	uboFS[0].rayDir = RAY_DIRECTION;
	uboFS[0].sunDir = SUN_DIRECTION;
	uboFS[0].enableFog = p_vkEngine->drawFog();
	uboFS[0].exposure = p_vkEngine->exposureSetting();
	uboFS[0].gamma = p_vkEngine->gammaSetting();

	p_vkMemory->MapDataToSegment<FogUboFS>(m_finalInfo.uboBuffer, uboFS);

	// vertex shader
	std::vector<PostProcessVS> uboVS(1);
	uboVS[0].projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
	uboVS[0].viewMatrix = glm::mat4(1.0f);
	uboVS[0].modelMatrix = glm::mat4(1.0f);
	uboVS[0].camModelView = camera->m_cameraInfo.viewMatrix * glm::mat4(1.0f);

	p_vkMemory->MapDataToSegment<PostProcessVS>(m_vertBuffer, uboVS);
}

void VkPostProcess::Init()
{
	offscreen_semaphore = VulkanUtility::CreateSemaphore(p_vkEngine->GetDevice());
	
	// create ubo buffers
	PrepareBuffers();

	PrepareFullscreenQuad();

	// setup buffers for HDR pass and blur
	PrepareFrameBuffers();

	// create descriptors for each pass
	PrepareColourPassDescriptors();
	PrepareBlurPassDescriptors();
	PrepareFinalDescriptors();

	// prepare piplines for all post-process components
	PreparePipelines();
}

void VkPostProcess::Destroy()
{
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_finalInfo.pipelineInfo.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_finalInfo.pipelineInfo.layout, nullptr);

	delete m_finalInfo.descriptors;

	p_vkEngine = nullptr;
}

// shader setup
VkVertexInputBindingDescription VkPostProcess::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 2> VkPostProcess::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 2> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(Vertex, uv);

	return attrDescr;
}
