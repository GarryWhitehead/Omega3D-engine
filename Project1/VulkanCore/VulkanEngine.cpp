#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_model.h"
#include "VulkanCore/vulkan_terrain.h"
#include "VulkanCore/vulkan_shadow.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/VulkanAnimation.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/VulkanPBR.h"
#include "VulkanCore/VulkanIBL.h"
#include "VulkanCore/VulkanSkybox.h"
#include "utility/file_log.h"


VulkanEngine::VulkanEngine(GLFWwindow *window) :
	VulkanCore(window),
	drawStateChanged(true),
	vk_prepared(false)
{	
	vkUtility = new VulkanUtility();
	vkUtility->InitVulkanUtility(this);
}

VulkanEngine::~VulkanEngine()
{
}

void VulkanEngine::RegisterVulkanModules(std::vector<VkModId> modules)
{
	for (auto mod : modules) {

		if (mod == VkModId::VKMOD_SHADOW_ID) {
			VulkanShadow *vkMod = new VulkanShadow(this, vkUtility);
			vkMod->Init();
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_SHADOW_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_PBR_ID) {
			VulkanPBR *vkMod = new VulkanPBR(this, vkUtility);
			vkMod->Init();
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_PBR_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_IBL_ID) {
			VulkanIBL *vkMod = new VulkanIBL(this, vkUtility);
			vkMod->Init();
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_IBL_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_SKYBOX_ID) {
			VulkanSkybox *vkMod = new VulkanSkybox(this, vkUtility);
			vkMod->Init();
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_SKYBOX_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_DEFERRED_ID) {
			VulkanDeferred *vkMod = new VulkanDeferred(this, vkUtility);
			vkMod->Init();
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_DEFERRED_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_TERRAIN_ID) {
			VulkanTerrain *vkMod = new VulkanTerrain(this, vkUtility);
			vkMod->Init();
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_TERRAIN_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_MODEL_ID) {
			VulkanModel *vkMod = new VulkanModel(this, vkUtility);
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_MODEL_ID, vkMod));
		}
		else if (mod == VkModId::VKMOD_ANIM_ID) {
			VulkanAnimation *vkMod = new VulkanAnimation(this, vkUtility);
			m_vkModules.insert(std::make_pair(VkModId::VKMOD_ANIM_ID, vkMod));
		}
	}
}

TextureInfo VulkanEngine::InitDepthImage()
{
	// required depth image format in order of preference
	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

	m_depthImageFormat = vkUtility->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	TextureInfo depthImage;
	depthImage.width = m_surface.extent.width;
	depthImage.height = m_surface.extent.height;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = m_depthImageFormat;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = depthImage.width;
	image_info.extent.height = depthImage.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(m_device.device, &image_info, nullptr, &depthImage.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(m_device.device, depthImage.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(m_device.device, &alloc_info, nullptr, &depthImage.texture_mem));

	vkBindImageMemory(m_device.device, depthImage.image, depthImage.texture_mem, 0);

	depthImage.imageView = InitImageView(depthImage.image, m_depthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);

	vkUtility->ImageTransition(VK_NULL_HANDLE, depthImage.image, m_depthImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, m_cmdPool);

	return depthImage;
}

void VulkanEngine::PrepareRenderpass()
{
	VkAttachmentDescription colorAttach = {};
	colorAttach.format = m_surface.format.format;
	colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttach = {};
	depthAttach.format = m_depthImageFormat;
	depthAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDependency sPassDepend = {};
	sPassDepend.srcSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend.dstSubpass = 0;
	sPassDepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend.srcAccessMask = 0;
	sPassDepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDescription sPassDescr = {};
	sPassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sPassDescr.colorAttachmentCount = 1;
	sPassDescr.pColorAttachments = &colorRef;
	sPassDescr.pDepthStencilAttachment = &depthRef;

	std::vector<VkAttachmentDescription> attach = { colorAttach, depthAttach };
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attach.size());
	createInfo.pAttachments = attach.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sPassDescr;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &sPassDepend;

	VK_CHECK_RESULT(vkCreateRenderPass(m_device.device, &createInfo, nullptr, &m_renderpass));
}

void VulkanEngine::PrepareFrameBuffers()
{
	m_frameBuffer = vkUtility->InitFrameBuffers(m_surface.extent.width, m_surface.extent.height, m_renderpass, m_depthImage.imageView);
}

void VulkanEngine::RenderScene(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	if (pipeline == VK_NULL_HANDLE) {
	
		VkModule<VulkanSkybox>(VkModId::VKMOD_SKYBOX_ID)->GenerateSkyboxCmdBuffer(cmdBuffer);
	}

	VkModule<VulkanTerrain>(VkModId::VKMOD_TERRAIN_ID)->GenerateTerrainCmdBuffer(cmdBuffer, set, layout, pipeline);
	VkModule<VulkanAnimation>(VkModId::VKMOD_ANIM_ID)->GenerateModelCmdBuffer(cmdBuffer, set, layout, pipeline);
	VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID)->GenerateModelCmdBuffer(cmdBuffer, set, layout, pipeline);
}

void VulkanEngine::Init()
{
	// Initialise the "global" renderpass and framebuffer which will be used by most systems
	// image processing modules will use their own offscreen framebuffers though usually the same renderpass
	m_cmdPool = vkUtility->InitCommandPool(m_queue.graphIndex);

	// add depth test image used by the renderpass
	m_depthImage = InitDepthImage();

	PrepareRenderpass();
	PrepareFrameBuffers();
}

void VulkanEngine::DrawScene()
{
	// generate BDRF lut for PBR
	VkModule<VulkanPBR>(VkModId::VKMOD_PBR_ID)->GenerateLUTCmdBuffer();

	// generate irradiance and prefilter maps for environment cube
	VkModule<VulkanIBL>(VkModId::VKMOD_IBL_ID)->GenerateIrrMapCmdBuffer();
	VkModule<VulkanIBL>(VkModId::VKMOD_IBL_ID)->GeneratePreFilterCmdBuffer();
	
	// command buffer for shadow and deferred draws
	m_offscreenCmdBuffer = vkUtility->CreateCmdBuffer(vkUtility->VK_PRIMARY, vkUtility->VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, m_cmdPool);

	// first pass - scene is drawn into shadow buffer
	VkModule<VulkanShadow>(VkModId::VKMOD_SHADOW_ID)->GenerateShadowCmdBuffer(m_offscreenCmdBuffer);

	// second pass - scene is drawn into deferred buffers - position, albedo and normal. This data is then 
	// passed into the G buffer
	VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID)->GenerateDeferredCmdBuffer(m_offscreenCmdBuffer);

	VK_CHECK_RESULT(vkEndCommandBuffer(m_offscreenCmdBuffer));

	// scene is drawn as a full screen quad and lighting calculation are done in the shader
	VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID)->GenerateFullscreenCmdBuffers();
	
	vk_prepared = true;
}

void VulkanEngine::Update(CameraSystem *camera)
{
	VkModule<VulkanShadow>(VkModId::VKMOD_SHADOW_ID)->Update(camera);
	VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID)->Update(camera);
	VkModule<VulkanSkybox>(VkModId::VKMOD_SKYBOX_ID)->Update(camera);
	VkModule<VulkanTerrain>(VkModId::VKMOD_TERRAIN_ID)->Update(camera);
	VkModule<VulkanAnimation>(VkModId::VKMOD_ANIM_ID)->Update(camera);
	VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID)->Update(camera);
}

void VulkanEngine::Render()
{
	//check whether the draw buffers have changed and re-generate cmd buffers if this is the case
	// also check whether anything has been drawn yet
	if (drawStateChanged) {

		drawStateChanged = false;
		DrawScene();
		
	}

	if (vk_prepared) {

		SubmitFrame();
	}
}

void VulkanEngine::SubmitFrame()
{
	uint32_t imageIndex = vkUtility->InitRenderFrame();
	auto vkShadow = VkModule<VulkanShadow>(VkModId::VKMOD_SHADOW_ID);
	auto vkDeferred = VkModule<VulkanDeferred>(VkModId::VKMOD_DEFERRED_ID);

	VkSubmitInfo submit_info = {};
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.pWaitDstStageMask = &flags;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &m_semaphore.image;											// wait for swap chain presentation to finish
	submit_info.pSignalSemaphores = &vkShadow->m_shadowInfo.semaphore;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_offscreenCmdBuffer;
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	submit_info.pWaitSemaphores = &vkShadow->m_shadowInfo.semaphore;
	submit_info.pSignalSemaphores = &m_semaphore.render;

 	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vkDeferred->m_cmdBuffers[imageIndex];
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	vkUtility->SubmitFrame(imageIndex);
}

VulkanModel* VulkanEngine::AssociateWithVulkanModel(ModelResourceManager* manager)
{
	VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID)->p_modelManager = manager;
	auto model = VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID);
	assert(model != nullptr);
	return model;
}

VulkanAnimation*  VulkanEngine::AssociateWithVulkanAnimation(ModelResourceManager* manager)
{
	VkModule<VulkanAnimation>(VkModId::VKMOD_ANIM_ID)->p_modelManager = manager;
	auto anim = VkModule<VulkanAnimation>(VkModId::VKMOD_ANIM_ID);
	assert(anim != nullptr);
	return anim;
}

