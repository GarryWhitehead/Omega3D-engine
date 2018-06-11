#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanModel.h"
#include "VulkanCore/vulkan_terrain.h"
#include "VulkanCore/vulkan_shadow.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/VulkanDeferred.h"
#include "VulkanCore/VulkanPBR.h"
#include "VulkanCore/VulkanIBL.h"
#include "VulkanCore/VulkanSkybox.h"
#include "VulkanCore/VulkanWater.h"
#include "VulkanCore/VkMemoryManager.h"
#include "VulkanCore/VkPostProcess.h"
#include "VulkanCore/VulkanGUI.h"
#include "Systems/input_system.h"
#include "Engine/World.h"
#include "utility/file_log.h"


VulkanEngine::VulkanEngine(GLFWwindow *window, MessageHandler *msgHandler) :
	VulkanCore(window),
	p_message(msgHandler),
	drawStateChanged(true),
	vk_prepared(false),
	displayGUI(true)
{	
	vkUtility = new VulkanUtility(this);
	p_vkMemory = new VkMemoryManager(this);
}

VulkanEngine::~VulkanEngine()
{
}

void VulkanEngine::RegisterVulkanModules(std::vector<VkModId> modules)
{
	for (auto mod : modules) {

		switch (mod) {
			case VkModId::VKMOD_SHADOW_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanShadow)), new VulkanShadow(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_PBR_ID: 
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanPBR)), new VulkanPBR(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_IBL_ID:				
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanIBL)), new VulkanIBL(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_SKYBOX_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanSkybox)), new VulkanSkybox(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_DEFERRED_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanDeferred)), new VulkanDeferred(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_TERRAIN_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanTerrain)), new VulkanTerrain(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_WATER_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanWater)), new VulkanWater(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_MODEL_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VulkanModel)), new VulkanModel(this, vkUtility, p_vkMemory)));
				break;
			case VkModId::VKMOD_POSTPROCESS_ID:
				m_vkModules.insert(std::make_pair(std::type_index(typeid(VkPostProcess)), new VkPostProcess(this, vkUtility, p_vkMemory)));
				break;
		}
	}
}

void VulkanEngine::RenderScene(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	if (hasModule<VulkanTerrain>() && p_vkGUI->m_guiSettings.terrainType == 1) {
		VkModule<VulkanTerrain>()->GenerateTerrainCmdBuffer(cmdBuffer, set, layout, pipeline);
	}

	if (hasModule<VulkanWater>() && p_vkGUI->m_guiSettings.terrainType == 0) {
		VkModule<VulkanWater>()->GenerateWaterCmdBuffer(cmdBuffer, set, layout, pipeline);
	}

	if (hasModule<VulkanModel>()) {
		VkModule<VulkanModel>()->GenerateModelCmdBuffer(cmdBuffer, set, layout, pipeline); 
	}
}

void VulkanEngine::PrepareFinalFrameBuffer(bool prepareFrameBufferOnly)
{
	uint32_t width = m_surface.extent.width;
	uint32_t height = m_surface.extent.height;

	if (!prepareFrameBufferOnly) {

		// depth image
		std::vector<VkFormat> formats =
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};

		VkFormat depthFormat = vkUtility->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
		m_depthImage.PrepareImage(depthFormat, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height, this, false);

		// create presentation renderpass/framebuffer
		m_renderpass.AddAttachment(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, m_surface.format.format);
		m_renderpass.AddAttachment(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, depthFormat);

		m_renderpass.AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
		m_renderpass.AddReference(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
		m_renderpass.PrepareRenderPass(m_device.device);

	}

	std::array<VkImageView, 2> attachments = {};
	m_frameBuffers.resize(m_swapchain.imageCount);

	for (uint32_t c = 0; c < m_frameBuffers.size(); ++c) {

		attachments[0] = m_imageView.images[c];
		attachments[1] = m_depthImage.imageView;

		VkFramebufferCreateInfo frameInfo = {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = m_renderpass.renderpass;
		frameInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameInfo.pAttachments = attachments.data();
		frameInfo.width = width;
		frameInfo.height = height;
		frameInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(m_device.device, &frameInfo, nullptr, &m_frameBuffers[c]));
	}
}

void VulkanEngine::GenerateFinalCmdBuffer()
{
	// if we have already generated a commnad buffer but are re-drawing, then free the present buffer
	if (!m_cmdBuffers.empty()) {

		for (uint32_t c = 0; c < m_cmdBuffers.size(); ++c) {
			vkFreeCommandBuffers(m_device.device, m_cmdPool, 1, &m_cmdBuffers[c]);
		}
	}

	// final cmd buffer in the chain - images will be rendered onto presentation surface (i.e the screen)
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	m_cmdBuffers.resize(m_frameBuffers.size());

	for (uint32_t c = 0; c < m_cmdBuffers.size(); ++c) {

		m_cmdBuffers[c] = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, m_cmdPool);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = m_frameBuffers[c];
		renderPassInfo.renderPass = m_renderpass.renderpass;
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent.width = m_surface.extent.width;
		renderPassInfo.renderArea.extent.height = m_surface.extent.height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_cmdBuffers[c], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// draw screen with post screen processing as a full screen quad
		VkModule<VkPostProcess>()->GenerateCmdBuffer(m_cmdBuffers[c]);

		// render GUI on top of image - if seleceted
		if (displayGUI) {

			p_vkGUI->GenerateCmdBuffer(m_cmdBuffers[c], p_vkMemory);
		}

		vkCmdEndRenderPass(m_cmdBuffers[c]);
		VK_CHECK_RESULT(vkEndCommandBuffer(m_cmdBuffers[c]));
	}
}

void VulkanEngine::DrawScene()
{
	vk_prepared = false;

	// generate BDRF lut for PBR
	VkModule<VulkanPBR>()->GenerateLUTCmdBuffer();

	// generate irradiance and prefilter maps for environment cube
	VkModule<VulkanIBL>()->GenerateIrrMapCmdBuffer();
	VkModule<VulkanIBL>()->GeneratePreFilterCmdBuffer();
	
	// draw shadow image into offscreen buffer
	VkModule<VulkanShadow>()->GenerateShadowCmdBuffer();
	
	// draw deferred image into offscreen buffer
	VkModule<VulkanDeferred>()->DrawDeferredScene();

	// apply post processing and draw to presentation image
	GenerateFinalCmdBuffer();
	
	vk_prepared = true;		// ensure everyting has been initialised before submitting to the relevant queue
}

void VulkanEngine::SubmitFrame()
{
	uint32_t imageIndex = vkUtility->InitRenderFrame();		// current swapcahin presentation index
	
	// out of date swap chains are usually due to a window resize
	if (imageIndex == VK_ERROR_OUT_OF_DATE_KHR) {

		PrepareNewSwapFrame();
		return;
	}

	auto vkShadow = VkModule<VulkanShadow>();
	auto vkDeferred = VkModule<VulkanDeferred>();
	auto vkPProcess = VkModule<VkPostProcess>();

	// update the GUI IO before preceding
	p_vkGUI->Update();

	// update final command buffer 
	GenerateFinalCmdBuffer();

	if (hasModule<VulkanWater>()) {
		auto vkWater = VkModule<VulkanWater>();

		// submit compute shaders for spectrum, fft and displacement map computations
		vkWater->SubmitWaterCompute();
	}

	VkSubmitInfo submit_info = {};
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.pWaitDstStageMask = &flags;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.signalSemaphoreCount = 1;
	submit_info.commandBufferCount = 1;

	// wait for the shadow offscreen buffer to finish	
	submit_info.pWaitSemaphores = &m_semaphore.image;
	submit_info.pSignalSemaphores = &vkShadow->m_shadowInfo.semaphore;
	submit_info.pCommandBuffers = &vkShadow->m_cmdBuffer;
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	// wait for the deferred offscreen buffer
	submit_info.pWaitSemaphores = &vkShadow->m_shadowInfo.semaphore;
	submit_info.pSignalSemaphores = &vkDeferred->m_deferredInfo.semaphore;
	submit_info.pCommandBuffers = &vkDeferred->m_deferredInfo.cmdBuffer;
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	// post process/swap chain presentation submit
	submit_info.pWaitSemaphores = &vkDeferred->m_deferredInfo.semaphore;
	submit_info.pSignalSemaphores = &m_semaphore.render;
	submit_info.pCommandBuffers = &m_cmdBuffers[imageIndex];
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	vkUtility->SubmitFrame(imageIndex);
}

void VulkanEngine::Render()
{
	//check whether the draw buffers have changed and re-generate cmd buffers if this is the case
	// also check whether anything has been drawn yet
	if (drawStateChanged) {

		DrawScene();
		drawStateChanged = false;
	}

	if (vk_prepared) {

		SubmitFrame();
	}
}

void VulkanEngine::Update(int acc_time)
{
	for (auto &mod : m_vkModules) {

		mod.second->Update(acc_time);
	}

	// check whether there are any messages for vulkan
	p_message->Notify();
}

void VulkanEngine::Init(World *world)
{
	p_world = world;

	// Initialise the  graphics command pool used by all modules at present
	m_cmdPool = vkUtility->InitCommandPool(m_queue.graphIndex);

	// and a compute command pool
	m_computeCmdPool = vkUtility->InitCommandPool(m_queue.computeIndex);

	// setup some initial memory blocks - one large 256mb block for static local data and one smaller 
	// host-visible memory block for dynamic buffers such as uniform buffers
	p_vkMemory->AllocateBlock(MemoryType::VK_BLOCK_TYPE_LOCAL);
	p_vkMemory->AllocateBlock(MemoryType::VK_BLOCK_TYPE_HOST);

	// prepare renderpass and framebuffer for final render pass
	PrepareFinalFrameBuffer(false);

	// prepare GUI
	p_vkGUI = new VulkanGUI(this);
	p_vkGUI->Init(p_vkMemory);				// initialise font texture smaplers and styles along with descriptors/pipeline

	// register with the message handling system
	p_message->AddListener(ListenerID::VULKAN_MSG, NotifyResponse());
}

void VulkanEngine::Destroy()
{
	// destroy all vulkan related stuff to do with the engine
	DestroyPresentation();

	// destroy all modules
	for (auto mod : m_vkModules) {
		delete mod.second;				// all modules will be destroyed via the destructor
	}

	m_depthImage.Destroy(m_device.device);
	vkDestroyCommandPool(m_device.device, m_cmdPool, nullptr);			// graphics cmd pool
	vkDestroyCommandPool(m_device.device, m_computeCmdPool, nullptr);	// compute cmd pool

	delete p_vkGUI;
	delete p_vkMemory;		// will de-allocate all mem blocks on destruction
	delete vkUtility;

	// tidy up the shared poiners
	p_world = nullptr;
	p_graphicsSystem = nullptr;
	p_message = nullptr;
	p_vkGUI = nullptr;
	p_vkMemory = nullptr;
}

void VulkanEngine::DestroyPresentation()
{
	for (uint32_t c = 0; c < m_frameBuffers.size(); ++c) {
		vkDestroyFramebuffer(m_device.device, m_frameBuffers[c], nullptr);
	}

	vkFreeCommandBuffers(m_device.device, m_cmdPool, static_cast<uint32_t>(m_cmdBuffers.size()), m_cmdBuffers.data());
}

void VulkanEngine::PrepareNewSwapFrame()
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(m_device.device));
	
	// destroy frame/command buffers associated with the swapchain
	DestroyPresentation();
	DestroySwapChain();			// destroy the actual swapchain and image views

	// re-initialise the swap chain 
	InitSwapChain();

	// and re-initialise the framebuffers and cmdbuffers
	PrepareFinalFrameBuffer(true);
}

// message handling functions ====================================================================================================================================================================

void VulkanEngine::OnNotify(Message& msg)
{
	if (msg.message == "SwitchGUI") {

		displayGUI = (displayGUI) ? false : true;

		// if HUD enabled - enable mouse pointer, otherwise disable;	
		p_world->RequestSystem<InputSystem>()->SwitchWindowCursorState();
		
		drawStateChanged = true;				// regenerate the cmd buffers
	}
}

std::function<void(Message)> VulkanEngine::NotifyResponse()
{
	auto msgListener = [=](Message msg) -> void
	{
		this->OnNotify(msg);
	};

	return msgListener;
}

void VulkanEngine::SendMessage(Message msg)
{
	p_message->AddMessage(msg);
}

// functions for the various GUI settings ====================================================================================================================================================

bool VulkanEngine::drawWireframe() const 
{ 
	return p_vkGUI->m_guiSettings.wireframe; 
}

uint32_t VulkanEngine::drawFog() const
{
	return (p_vkGUI->m_guiSettings.showFog == true) ? 1 : 0;
}

bool VulkanEngine::displayLights() const
{
	return p_vkGUI->m_guiSettings.lights;
}

int VulkanEngine::terrainType() const
{
	return p_vkGUI->m_guiSettings.terrainType;
}

float VulkanEngine::waveAmplitude() const
{
	return p_vkGUI->m_guiSettings.amplitude;
}

float VulkanEngine::choppyFactor() const
{
	return p_vkGUI->m_guiSettings.choppiness;
}

float VulkanEngine::tesselationFactor() const
{
	return p_vkGUI->m_guiSettings.tesselation;
}

float VulkanEngine::displacementFactor() const
{
	return p_vkGUI->m_guiSettings.displacement;
}

float VulkanEngine::tessEdgeSize() const
{
	return p_vkGUI->m_guiSettings.edgeFactor;
}




