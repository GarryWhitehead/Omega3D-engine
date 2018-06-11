#include "VulkanDeferred.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/Vulkan_shadow.h"
#include "VulkanCore/VulkanSkybox.h"
#include "VulkanCore/VulkanPBR.h"
#include "VulkanCore/VulkanIBL.h"
#include "Systems/camera_system.h"
#include "Engine/World.h"
#include "Engine/engine.h"
#include "ComponentManagers/LightComponentManager.h"
#include <gtc/matrix_transform.hpp>

VulkanDeferred::VulkanDeferred(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory) :
	VulkanModule(utility, memory),
	p_vkEngine(engine)
{
	Init();
}

VulkanDeferred::~VulkanDeferred()
{
	Destroy();
}

void VulkanDeferred::PrepareDeferredImages()
{
	uint32_t width = p_vkEngine->GetSurfaceExtentW();
	uint32_t height = p_vkEngine->GetSurfaceExtentH();

	// initialise the colour buffer attachments for PBR
	m_deferredInfo.offscreen.imageInfo.PrepareImage(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, width, height, p_vkEngine, false);			// offscreen buffer
	m_deferredInfo.position.imageInfo.PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);		// positions
	m_deferredInfo.normal.imageInfo.PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);			// normals
	m_deferredInfo.albedo.imageInfo.PrepareImage(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);				// albedo colour buffer
	m_deferredInfo.bump.imageInfo.PrepareImage(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);				// bump colour buffer
	m_deferredInfo.ao.imageInfo.PrepareImage(VK_FORMAT_R16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);						// ao 
	m_deferredInfo.metallic.imageInfo.PrepareImage(VK_FORMAT_R16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);				// metallic 
	m_deferredInfo.roughness.imageInfo.PrepareImage(VK_FORMAT_R16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, width, height, p_vkEngine, false);				// roughness 

	// initialise the G buffer
	// required depth image format in order of preference
	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormat depthFormat = vkUtility->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	m_deferredInfo.depth.imageInfo.PrepareImage(depthFormat, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height, p_vkEngine, false);

}

void VulkanDeferred::PrepareDeferredFramebuffer()
{
	// first create the images for each G buffer
	PrepareDeferredImages();
	
	// Create renderpass attachment info for all G buffers
	std::array<VkAttachmentDescription, 9> attachDescr = {};
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.offscreen.imageInfo.format);			// offscreen colour buffer	
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.position.imageInfo.format);			// position
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.normal.imageInfo.format);				// normal
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.albedo.imageInfo.format);				//	albedo
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.bump.imageInfo.format);				//	bump
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.ao.imageInfo.format);					//	ao
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.metallic.imageInfo.format);			//	metallic
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.roughness.imageInfo.format);			//	roughness
	m_deferredInfo.renderpass.AddAttachment(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_deferredInfo.depth.imageInfo.format);		// depth


	VkAttachmentReference depthRef = { 8, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

	// ================================ subpass one - fill G-buffers
	std::vector<VkAttachmentReference> colorRef1 = 
	{
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// offscreen colour buffer
		{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// position
		{ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// normal
		{ 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// albedo
		{ 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// bump/normal
		{ 5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// ao
		{ 6, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// metallic
		{ 7, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },		// roughness
		
	};
	m_deferredInfo.renderpass.AddSubPass(colorRef1, &depthRef);

	// =============================== subpass two - draw scene into offscreen
	std::vector<VkAttachmentReference> colorRef2 = 
	{
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },	
	};

	std::vector<VkAttachmentReference> inputRef = 
	{
		{ 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// position
		{ 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// normal
		{ 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// albedo
		{ 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// bump
		{ 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// ao
		{ 6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },		// metallic
		{ 7, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }			// roughness
	};
	m_deferredInfo.renderpass.AddSubPass(colorRef2, inputRef, &depthRef);

	// =============================== subpass three - draw skybox 
	std::vector<VkAttachmentReference> colorRef3 =
	{
		{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL },
	};
	m_deferredInfo.renderpass.AddSubPass(colorRef3, &depthRef);

	// ================================ sub-pass dependencies
	m_deferredInfo.renderpass.AddSubpassDependency(DependencyTemplate::TEMPLATE_TOP_OF_PIPE);
	m_deferredInfo.renderpass.AddSubpassDependency(DependencyTemplate::TEMPLATE_MULTI_SUBPASS, 0, 1);		// G buffer to quad
	m_deferredInfo.renderpass.AddSubpassDependency(DependencyTemplate::TEMPLATE_MULTI_SUBPASS, 1, 2);		// quad to skybox
	m_deferredInfo.renderpass.AddSubpassDependency(DependencyTemplate::TEMPLATE_BOTTOM_OF_PIPE);

	m_deferredInfo.renderpass.PrepareRenderPass(p_vkEngine->GetDevice());

	// create frame buffer for offscreen
	std::vector<VkImageView> attachments = 
	{
		m_deferredInfo.offscreen.imageInfo.imageView,
		m_deferredInfo.position.imageInfo.imageView,
		m_deferredInfo.normal.imageInfo.imageView,
		m_deferredInfo.albedo.imageInfo.imageView,
		m_deferredInfo.bump.imageInfo.imageView,
		m_deferredInfo.ao.imageInfo.imageView,
		m_deferredInfo.metallic.imageInfo.imageView,
		m_deferredInfo.roughness.imageInfo.imageView,
		m_deferredInfo.depth.imageInfo.imageView
	};

	uint32_t width = p_vkEngine->GetSurfaceExtentW();
	uint32_t height = p_vkEngine->GetSurfaceExtentH();

	m_deferredInfo.renderpass.PrepareFramebuffer(attachments, width, height, p_vkEngine->GetDevice());
}

void VulkanDeferred::PrepareDeferredDescriptorSet()
{
	// deferred descriptor layouts
	std::vector<VkDescriptors::LayoutBinding> layouts =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// position
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// normal
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// albedo
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// bump
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// ao
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// metallic
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT },					// roughness
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },			// shadow
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },			// BDRF lut
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },			// irradiance map
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }				// pre-filter map
	};
	m_deferredInfo.descriptor.AddDescriptorBindings(layouts);

	auto vkShadow = p_vkEngine->VkModule<VulkanShadow>();
	auto vkPBR = p_vkEngine->VkModule<VulkanPBR>();
	auto vkIBL = p_vkEngine->VkModule<VulkanIBL>();

	std::vector<VkDescriptorBufferInfo> bufferInfo = 
	{
		{ p_vkMemory->blockBuffer(m_buffers.vertexUbo.block_id), m_buffers.vertexUbo.offset, m_buffers.vertexUbo.size },
		{ p_vkMemory->blockBuffer(m_buffers.fragmentUbo.block_id), m_buffers.fragmentUbo.offset, m_buffers.fragmentUbo.size }
	};

	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ VK_NULL_HANDLE, m_deferredInfo.position.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ VK_NULL_HANDLE, m_deferredInfo.normal.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ VK_NULL_HANDLE, m_deferredInfo.albedo.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ VK_NULL_HANDLE, m_deferredInfo.bump.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ VK_NULL_HANDLE, m_deferredInfo.ao.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ VK_NULL_HANDLE, m_deferredInfo.metallic.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ VK_NULL_HANDLE, m_deferredInfo.roughness.imageInfo.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ vkShadow->m_depthImage.texSampler, vkShadow->m_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL },
		{ vkPBR->m_lutImage.texSampler, vkPBR->m_lutImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ vkIBL->m_irradianceCube.cubeImage.texSampler, vkIBL->m_irradianceCube.cubeImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
		{ vkIBL->m_filterCube.cubeImage.texSampler, vkIBL->m_filterCube.cubeImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};
	m_deferredInfo.descriptor.GenerateDescriptorSets(bufferInfo.data(), imageInfo.data(), p_vkEngine->GetDevice());
	
}

void VulkanDeferred::PrepareDeferredPipeline()
{
	// offscreen pipeline
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

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility->InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	// colour attachment required for each colour buffer
	std::array<VkPipelineColorBlendAttachmentState, 1> colorAttach = {};
	colorAttach[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[0].blendEnable = VK_FALSE;

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
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(uint32_t);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_deferredInfo.descriptor.layout;
	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_deferredInfo.pipelineInfo.layout));

	// sahders for rendering full screen quad
	m_deferredInfo.shader[0] = vkUtility->InitShaders("deferred/deferred-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_deferredInfo.shader[1] = vkUtility->InitShaders("deferred/deferred-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_deferredInfo.shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_deferredInfo.pipelineInfo.layout;
	createInfo.renderPass = m_deferredInfo.renderpass.renderpass;
	createInfo.subpass = 1;						// complete scene draw pass
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_deferredInfo.pipelineInfo.pipeline));
}

void VulkanDeferred::GenerateDeferredCmdBuffer(VkCommandBuffer cmdBuffer)
{
	VkDeviceSize offsets[1]{ m_buffers.vertices.offset };

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredInfo.pipelineInfo.pipeline);
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_buffers.vertices.block_id), offsets);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_deferredInfo.pipelineInfo.layout, 0, 1, &m_deferredInfo.descriptor.set, 0, NULL);

	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_buffers.indices.block_id), m_buffers.indices.offset, VK_INDEX_TYPE_UINT32);

	auto p_lightManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();
	uint32_t lightCount = p_vkEngine->displayLights() ? p_lightManager->GetLightCount() : 0;
	vkCmdPushConstants(cmdBuffer, m_deferredInfo.pipelineInfo.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &lightCount);

	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
}

void VulkanDeferred::DrawDeferredScene()
{
	// if we have already generated a commnad buffer but are re-drawing, then free the present buffer
	if (m_deferredInfo.cmdBuffer != VK_NULL_HANDLE) {

		vkFreeCommandBuffers(p_vkEngine->GetDevice(), p_vkEngine->GetCmdPool(), 1, &m_deferredInfo.cmdBuffer);
	}
	
	VkClearColorValue colour = p_vkEngine->CLEAR_COLOR;

	std::array<VkClearValue, 9> clearValues = {};
	clearValues[0].color = colour;			// position
	clearValues[1].color = colour;			// normal
	clearValues[2].color = colour;			// albedo
	clearValues[3].color = colour;			// ao
	clearValues[4].color = colour;			// metallic
	clearValues[5].color = colour;			// roughness
	clearValues[6].color = colour;			// roughness
	clearValues[7].color = colour;			// roughness
	clearValues[8].depthStencil = { 1.0f, 0 };

	m_deferredInfo.cmdBuffer = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetCmdPool());

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.framebuffer = m_deferredInfo.renderpass.frameBuffer;
	renderPassInfo.renderPass = m_deferredInfo.renderpass.renderpass;
	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent.width = p_vkEngine->GetSurfaceExtentW();
	renderPassInfo.renderArea.extent.height = p_vkEngine->GetSurfaceExtentH();
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	VkViewport viewport = vkUtility->InitViewPort(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0.0f, 1.0f);
	vkCmdSetViewport(m_deferredInfo.cmdBuffer, 0, 1, &viewport);

	VkRect2D scissor = vkUtility->InitScissor(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH(), 0, 0);
	vkCmdSetScissor(m_deferredInfo.cmdBuffer, 0, 1, &scissor);

	vkCmdBeginRenderPass(m_deferredInfo.cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	// first pass - render the scene into the G buffers
	p_vkEngine->RenderScene(m_deferredInfo.cmdBuffer);

	// second pass - draw scene as a full screen quad
	vkCmdNextSubpass(m_deferredInfo.cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
	GenerateDeferredCmdBuffer(m_deferredInfo.cmdBuffer);

	// third pass - draw skybox : this is done so it won't be affected by lighting calculations
	if (p_vkEngine->hasModule<VulkanSkybox>()) {

		vkCmdNextSubpass(m_deferredInfo.cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
		p_vkEngine->VkModule<VulkanSkybox>()->GenerateSkyboxCmdBuffer(m_deferredInfo.cmdBuffer);
	}

	vkCmdEndRenderPass(m_deferredInfo.cmdBuffer);
	VK_CHECK_RESULT(vkEndCommandBuffer(m_deferredInfo.cmdBuffer));
}

void VulkanDeferred::CreateUBOBuffers()
{
	// vertex UBO buffer
	m_buffers.vertexUbo = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(VertexUBOLayout));

	// fragment UBO buffer
	m_buffers.fragmentUbo = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(FragmentUBOLayout));
}

void VulkanDeferred::PrepareFullscreenQuad()
{
	// prepare vertices
	std::vector<Vertex> vertices;
	vertices.push_back({ { 1.0f, 1.0f, 0.0f },{ 1.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });
	vertices.push_back({ { 0.0f, 1.0f, 0.0f },{ 0.0f, 1.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });
	vertices.push_back({ { 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });
	vertices.push_back({ { 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 1.0f, 1.0f, 1.0f } });

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
	m_buffers.vertices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(Vertex) * vertices.size());
	p_vkMemory->MapDataToSegment<Vertex>(m_buffers.vertices, vertices);

	// map indices
	m_buffers.indices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * indices.size());
	p_vkMemory->MapDataToSegment<uint32_t>(m_buffers.indices, indices);
}

void VulkanDeferred::Init()
{
	// create a semaphore to ensure that the deferred offscreen render has updated before generating on screen commands
	m_deferredInfo.semaphore = p_vkEngine->CreateSemaphore();

	PrepareDeferredFramebuffer();
	CreateUBOBuffers();
	PrepareFullscreenQuad();
	PrepareDeferredDescriptorSet();
	PrepareDeferredPipeline();
}

void VulkanDeferred::Update(int acc_time)
{
	auto vkShadow = p_vkEngine->VkModule<VulkanShadow>();
	auto p_lightManager = p_vkEngine->GetCurrentWorld()->RequestComponentManager<LightComponentManager>();
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	// update vertex ubo buffer
	std::vector<VertexUBOLayout> vertUbo(1);
	vertUbo[0].projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
	vertUbo[0].viewMatrix = glm::mat4(1.0f);
	vertUbo[0].modelMatrix = glm::mat4(1.0f);

	p_vkMemory->MapDataToSegment<VertexUBOLayout>(m_buffers.vertexUbo, vertUbo);

	// update fragment ubo buffer
	std::vector<FragmentUBOLayout> fragUbo(1);

	uint32_t lightCount = p_lightManager->GetLightCount();
	fragUbo[0].cameraPos = glm::vec4(camera->GetCameraPosition(), 1.0f);
	
	// light count depends on whether light seeting is activated
	fragUbo[0].activeLightCount = (p_vkEngine->displayLights()) ? lightCount : 0;

	for (uint32_t c = 0; c < lightCount; ++c) {

		LightComponentManager::LightInfo lightInfo = p_lightManager->GetLightData(c);
		fragUbo[0].light[c].pos = lightInfo.pos;
		fragUbo[0].light[c].direction = lightInfo.target;
		fragUbo[0].light[c].colour = lightInfo.colour;
	}

	p_vkMemory->MapDataToSegment<FragmentUBOLayout>(m_buffers.fragmentUbo, fragUbo);
}

void VulkanDeferred::Destroy()
{
	// destroy pipeline and decriptors
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_deferredInfo.pipelineInfo.pipeline, nullptr);
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_deferredInfo.pipelineInfo.layout, nullptr);
	vkDestroyDescriptorSetLayout(p_vkEngine->GetDevice(), m_deferredInfo.descriptor.set, nullptr);

	// deallocate sgements - memory blocks will be dalloacted upon closure of vkEngine
	p_vkMemory->DestroySegment(m_buffers.fragmentUbo);
	p_vkMemory->DestroySegment(m_buffers.indices);
	p_vkMemory->DestroySegment(m_buffers.vertexUbo);
	p_vkMemory->DestroySegment(m_buffers.vertices);
}

// shader setup
VkVertexInputBindingDescription VulkanDeferred::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 4> VulkanDeferred::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 4> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(Vertex, uv);

	// Vertex layout 2: normal
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(Vertex, normal);

	// Vertex layout 3: colour
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(Vertex, colour);

	return attrDescr;
}
