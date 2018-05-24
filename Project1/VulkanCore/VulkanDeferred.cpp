#include "VulkanDeferred.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/Vulkan_shadow.h"
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
}

void VulkanDeferred::CreateRenderpassAttachmentInfo(VkImageLayout finalLayout, VkFormat format, const uint32_t attachCount, VkAttachmentDescription *attachDescr)
{
	attachDescr->format = format;
	attachDescr->samples = VK_SAMPLE_COUNT_1_BIT;			// used for MSAA
	attachDescr->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachDescr->storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// IMPORTANT: this needs to be set to store operations for this to work!!!
	attachDescr->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachDescr->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachDescr->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachDescr->finalLayout = finalLayout;
}

void VulkanDeferred::PrepareDeferredFramebuffer()
{
	uint32_t width = p_vkEngine->GetSurfaceExtentW();
	uint32_t height = p_vkEngine->GetSurfaceExtentH();

	// initialise the colour buffer attachments for PBR
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

	// prepare deferred render pass
	PrepareDeferredRenderpass();

	// frame buffers for eachg swap chain
	std::array<VkImageView, 9> attachments = {};
	m_deferredInfo.frameBuffers.resize(p_vkEngine->GetSwapChainImageCount());
	
	for (uint32_t c = 0; c < m_deferredInfo.frameBuffers.size(); ++c) {

		attachments[0] = p_vkEngine->GetImageView(c);
		attachments[1] = m_deferredInfo.position.imageInfo.imageView;
		attachments[2] = m_deferredInfo.normal.imageInfo.imageView;
		attachments[3] = m_deferredInfo.albedo.imageInfo.imageView;
		attachments[4] = m_deferredInfo.bump.imageInfo.imageView;
		attachments[5] = m_deferredInfo.ao.imageInfo.imageView;
		attachments[6] = m_deferredInfo.metallic.imageInfo.imageView;
		attachments[7] = m_deferredInfo.roughness.imageInfo.imageView;
		attachments[8] = m_deferredInfo.depth.imageInfo.imageView;

		VkFramebufferCreateInfo frameInfo = {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = m_deferredInfo.renderPass;
		frameInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameInfo.pAttachments = attachments.data();
		frameInfo.width = p_vkEngine->GetSurfaceExtentW();
		frameInfo.height = p_vkEngine->GetSurfaceExtentH();
		frameInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->GetDevice(), &frameInfo, nullptr, &m_deferredInfo.frameBuffers[c]));
	}
}

void VulkanDeferred::PrepareDeferredRenderpass()
{
	// Create attachment info for colour attachment, G buffer 
	std::array<VkAttachmentDescription, 9> attachDescr = {};
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, p_vkEngine->GetSurfaceFormat(), 0, &attachDescr[0]);						// color attachment - for swap chain presentation
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.position.imageInfo.format, 1, &attachDescr[1]);			// position
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.normal.imageInfo.format, 2, &attachDescr[2]);			// normal
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.albedo.imageInfo.format, 3, &attachDescr[3]);			//	albedo
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.bump.imageInfo.format, 4, &attachDescr[4]);				//	bump
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.ao.imageInfo.format, 5, &attachDescr[5]);				//	ao
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.metallic.imageInfo.format, 6, &attachDescr[6]);			//	metallic
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, m_deferredInfo.roughness.imageInfo.format, 7, &attachDescr[7]);		//	roughness
	CreateRenderpassAttachmentInfo(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_deferredInfo.depth.imageInfo.format, 8, &attachDescr[8]);	// depth

	std::array<VkSubpassDescription, 3> subpassDescr = {};

	// ================================ subpass one - fill G-buffers
	std::array<VkAttachmentReference, 8> colorRef1 = {};
	colorRef1[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// colour - swapchain present
	colorRef1[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// position
	colorRef1[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// normal
	colorRef1[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// albedo
	colorRef1[4] = { 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// bump/normal
	colorRef1[5] = { 5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// ao
	colorRef1[6] = { 6, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// metallic
	colorRef1[7] = { 7, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// roughness

	VkAttachmentReference depthRef = { 8, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
	subpassDescr[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescr[0].colorAttachmentCount = static_cast<uint32_t>(colorRef1.size());
	subpassDescr[0].pColorAttachments = colorRef1.data();
	subpassDescr[0].pDepthStencilAttachment = &depthRef;

	// =============================== subpass two - draw scene 
	std::array<VkAttachmentReference, 1> colorRef2 = {};
	colorRef2[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// colour - swapchain present

	std::array<VkAttachmentReference, 7> inputRef = {};
	inputRef[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// position
	inputRef[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// normal
	inputRef[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// albedo
	inputRef[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// bump
	inputRef[4] = { 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// ao
	inputRef[5] = { 6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// metallic
	inputRef[6] = { 7, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };		// roughness

	subpassDescr[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescr[1].colorAttachmentCount = static_cast<uint32_t>(colorRef2.size());
	subpassDescr[1].pColorAttachments = colorRef2.data();
	subpassDescr[1].pDepthStencilAttachment = &depthRef;
	subpassDescr[1].inputAttachmentCount = static_cast<uint32_t>(inputRef.size());
	subpassDescr[1].pInputAttachments = inputRef.data();

	// =============================== subpass three - draw skybox 
	std::array<VkAttachmentReference, 1> colorRef3 = {};
	colorRef3[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };		// colour - swapchain present

	subpassDescr[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescr[2].colorAttachmentCount = static_cast<uint32_t>(colorRef3.size());
	subpassDescr[2].pColorAttachments = colorRef3.data();
	subpassDescr[2].pDepthStencilAttachment = &depthRef;

	std::array<VkSubpassDependency, 4> sPassDepend = {};
	sPassDepend[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[0].dstSubpass = 0;
	sPassDepend[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	sPassDepend[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[1].srcSubpass = 0;
	sPassDepend[1].dstSubpass = 1;
	sPassDepend[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	sPassDepend[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	sPassDepend[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[2].srcSubpass = 1;
	sPassDepend[2].dstSubpass = 2;
	sPassDepend[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	sPassDepend[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	sPassDepend[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	sPassDepend[3].srcSubpass = 0;
	sPassDepend[3].dstSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	sPassDepend[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	sPassDepend[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	sPassDepend[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attachDescr.size());
	createInfo.pAttachments = attachDescr.data();
	createInfo.subpassCount = static_cast<uint32_t>(subpassDescr.size());
	createInfo.pSubpasses = subpassDescr.data();
	createInfo.dependencyCount = static_cast<uint32_t>(sPassDepend.size());
	createInfo.pDependencies = sPassDepend.data();

	VK_CHECK_RESULT(vkCreateRenderPass(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_deferredInfo.renderPass));
}

void VulkanDeferred::PrepareDeferredDescriptorSet()
{
	std::array<VkDescriptorPoolSize, 3> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 2;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descrPoolSize[1].descriptorCount = 6;
	descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	descrPoolSize[2].descriptorCount = 7;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_deferredInfo.descriptor.pool));

	// deferred descriptor layout
	std::array<VkDescriptorSetLayoutBinding, 13> layoutBind = {};
	layoutBind[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	layoutBind[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	layoutBind[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// position
	layoutBind[3] = vkUtility->InitLayoutBinding(3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// normal
	layoutBind[4] = vkUtility->InitLayoutBinding(4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// albedo
	layoutBind[5] = vkUtility->InitLayoutBinding(5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// bump
	layoutBind[6] = vkUtility->InitLayoutBinding(6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// ao
	layoutBind[7] = vkUtility->InitLayoutBinding(7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// metallic
	layoutBind[8] = vkUtility->InitLayoutBinding(8, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT);					// roughness
	layoutBind[9] = vkUtility->InitLayoutBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// shadow
	layoutBind[10] = vkUtility->InitLayoutBinding(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// BDRF lut
	layoutBind[11] = vkUtility->InitLayoutBinding(11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// irradiance map
	layoutBind[12] = vkUtility->InitLayoutBinding(12, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);			// pre-filter map

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBind.size());
	layoutInfo.pBindings = layoutBind.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_deferredInfo.descriptor.layout));

	// Create descriptor set for meshes
	VkDescriptorSetLayout layouts[] = { m_deferredInfo.descriptor.layout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_deferredInfo.descriptor.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_deferredInfo.descriptor.set));

	std::array<VkDescriptorBufferInfo, 2> uboBufferInfo = {};
	uboBufferInfo[0] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(m_buffers.vertexUbo.block_id), m_buffers.vertexUbo.offset, m_buffers.vertexUbo.size);
	uboBufferInfo[1] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(m_buffers.fragmentUbo.block_id), m_buffers.fragmentUbo.offset, m_buffers.fragmentUbo.size);

	auto vkShadow = p_vkEngine->VkModule<VulkanShadow>();
	auto vkPBR = p_vkEngine->VkModule<VulkanPBR>();
	auto vkIBL = p_vkEngine->VkModule<VulkanIBL>();

	std::array<VkDescriptorImageInfo, 11> imageInfo = {};
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.position.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[1] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.normal.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[2] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.albedo.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[3] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.bump.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[4] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.ao.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[5] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.metallic.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[6] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_deferredInfo.roughness.imageInfo.imageView, VK_NULL_HANDLE);
	imageInfo[7] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, vkShadow->m_depthImage.imageView, vkShadow->m_depthImage.texSampler);
	imageInfo[8] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkPBR->lutImage.imageView, vkPBR->lutImage.texSampler);
	imageInfo[9] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkIBL->m_irradianceCube.cubeImage.imageView, vkIBL->m_irradianceCube.cubeImage.texSampler);
	imageInfo[10] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, vkIBL->m_filterCube.cubeImage.imageView, vkIBL->m_filterCube.cubeImage.texSampler);
	
	std::array<VkWriteDescriptorSet, 13> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboBufferInfo[1]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[0]);
	writeDescrSet[3] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[1]);
	writeDescrSet[4] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[2]);
	writeDescrSet[5] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[3]);
	writeDescrSet[6] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[4]);
	writeDescrSet[7] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[5]);
	writeDescrSet[8] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 8, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &imageInfo[6]);
	writeDescrSet[9] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[7]);
	writeDescrSet[10] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[8]);
	writeDescrSet[11] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[9]);
	writeDescrSet[12] = vkUtility->InitDescriptorSet(m_deferredInfo.descriptor.set, 12, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &imageInfo[10]);
	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);
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
	createInfo.renderPass = m_deferredInfo.renderPass;
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
	uint32_t lightCount = p_lightManager->GetLightCount();
	vkCmdPushConstants(cmdBuffer, m_deferredInfo.pipelineInfo.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &lightCount);

	vkCmdDrawIndexed(cmdBuffer, 6, 1, 0, 0, 0);
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

	// map to device memory
	m_buffers.vertices.size = 
	m_buffers.indices.size = sizeof(uint32_t) * indices.size();

	// map vertices
	m_buffers.vertices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(Vertex) * vertices.size());
	p_vkMemory->MapDataToSegment<Vertex>(m_buffers.vertices, vertices);

	// map indices
	m_buffers.indices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * indices.size());
	p_vkMemory->MapDataToSegment<uint32_t>(m_buffers.indices, indices);
}

void VulkanDeferred::Init()
{
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
	fragUbo[0].activeLightCount = lightCount;

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
