#include "Rendering/RenderManager.h"


#include "Engine/World.h"
#include "Engine/Engine.h"

namespace OmegaEngine
{

	RenderManager::RenderManager(RenderConfig& config) :
		renderConfig(config)
	{
		render_interface = std::make_unique<RenderInterface>();
		
		

	}
	 RenderManager::~RenderManager()
	{

	}

}



/*

void VulkanEngine::PrepareFinalFrameBuffer(bool prepareFrameBufferOnly)
{
	uint32_t width = p_vkSwapChain->surfaceInfo.extent.width;
	uint32_t height = p_vkSwapChain->surfaceInfo.extent.height;

	if (!prepareFrameBufferOnly) {

		// depth image
		std::vector<VkFormat> formats =
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};

		VkFormat depthFormat = VulkanUtility::FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, p_vkDevice->physDevice);
		m_depthImage = new VulkanTexture(p_vkDevice->physDevice, p_vkDevice->device);
		m_depthImage->PrepareImage(depthFormat, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, width, height, 0, false);

		// create presentation renderpass/framebuffer
		m_renderpass = new VulkanRenderPass(p_vkDevice->device);
		m_renderpass->AddAttachment(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, p_vkSwapChain->surfaceInfo.format.format);
		m_renderpass->AddAttachment(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, depthFormat);

		m_renderpass->AddReference(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0);
		m_renderpass->AddReference(vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
		m_renderpass->PrepareRenderPass(p_vkDevice->device);
	}

	std::array<VkImageView, 2> attachments = {};
	m_frameBuffers.resize(p_vkSwapChain->imageCount);

	for (uint32_t c = 0; c < m_frameBuffers.size(); ++c) {

		attachments[0] = p_vkSwapChain->imageViews[c];
		attachments[1] = m_depthImage->imageView;

		VkFramebufferCreateInfo frameInfo = {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = m_renderpass->renderpass;
		frameInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameInfo.pAttachments = attachments.data();
		frameInfo.width = width;
		frameInfo.height = height;
		frameInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(p_vkDevice->device, &frameInfo, nullptr, &m_frameBuffers[c]));
	}
}

void VulkanEngine::GenerateFinalCmdBuffer()
{
	// if we have already generated a commnad buffer but are re-drawing, then free the present buffer
	if (!m_cmdBuffers.empty()) {

		for (uint32_t c = 0; c < m_cmdBuffers.size(); ++c) {
			vkFreeCommandBuffers(p_vkDevice->device, m_cmdPool, 1, &m_cmdBuffers[c]);
		}
	}

	// final cmd buffer in the chain - images will be rendered onto presentation surface (i.e the screen)
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = CLEAR_COLOR;
	clearValues[1].depthStencil = { 1.0f, 0 };

	m_cmdBuffers.resize(m_frameBuffers.size());

	for (uint32_t c = 0; c < m_cmdBuffers.size(); ++c) {

		m_cmdBuffers[c] = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, m_cmdPool, p_vkDevice->device);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = m_frameBuffers[c];
		renderPassInfo.renderPass = m_renderpass->renderpass;
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent.width = p_vkSwapChain->surfaceInfo.extent.width;
		renderPassInfo.renderArea.extent.height = p_vkSwapChain->surfaceInfo.extent.height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(m_cmdBuffers[c], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// draw screen with post screen processing as a full screen quad
		VkModule<VkPostProcess>()->GenerateFinalCmdBuffer(m_cmdBuffers[c]);

		// debugging overlay for shadows
		if (debugShadowSetting()) {

			VkModule<VkPostProcess>()->GenerateDebugCmdBuffer(m_cmdBuffers[c]);
		}

		// render GUI on top of all images - if seleceted
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
	
	// draw deferred image into hdr offscreen buffer
	VkModule<VulkanDeferred>()->DrawDeferredScene();

	// gnerate normal and bright hdr scene and apply bloom
	VkModule<VkPostProcess>()->DrawBloom();

	// final composition including colour pass, bloom and fog and draw to presentation image
	GenerateFinalCmdBuffer();
	
	vk_prepared = true;		// ensure everyting has been initialised before submitting to the relevant queue
}

void VulkanEngine::SubmitFrame()
{
	uint32_t imageIndex = VulkanUtility::InitRenderFrame(p_vkDevice->device, p_vkSwapChain->swapChain, p_vkSwapChain->semaphore.image);		// current swapcahin presentation index
	
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

	// wait for the deferred and shadow offscreen buffers
	submit_info.pWaitSemaphores = &p_vkSwapChain->semaphore.image;
	submit_info.pSignalSemaphores = &vkDeferred->m_deferredInfo.semaphore;
	submit_info.pCommandBuffers = &vkDeferred->m_deferredInfo.cmdBuffer;
	VK_CHECK_RESULT(vkQueueSubmit(p_vkDevice->queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	// render col/bright and blur into fbs
	VkModule<VkPostProcess>()->Submit(&vkDeferred->m_deferredInfo.semaphore);

	// final composition - includes fog effect
	submit_info.pWaitSemaphores = &VkModule<VkPostProcess>()->GetOffscreenSemaphore();		// final wait before on screen render
	submit_info.pSignalSemaphores = &p_vkSwapChain->semaphore.render;
	submit_info.pCommandBuffers = &m_cmdBuffers[imageIndex];
	VK_CHECK_RESULT(vkQueueSubmit(p_vkDevice->queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	VulkanUtility::SubmitFrame(imageIndex, p_vkSwapChain->swapChain, p_vkSwapChain->semaphore.render, p_vkDevice->queue.presentQueue);
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
	m_cmdPool = VulkanUtility::InitCommandPool(p_vkDevice->queue.graphIndex, p_vkDevice->device);

	// and a compute command pool
	m_computeCmdPool = VulkanUtility::InitCommandPool(p_vkDevice->queue.computeIndex, p_vkDevice->device);

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

	vkDestroyCommandPool(p_vkDevice->device, m_cmdPool, nullptr);			// graphics cmd pool
	vkDestroyCommandPool(p_vkDevice->device, m_computeCmdPool, nullptr);	// compute cmd pool

	delete p_vkGUI;
	delete p_vkMemory;		// will de-allocate all mem blocks on destruction

	// delete the core
	delete p_vkSwapChain;

	// destroy the validation layers
	ValidationLayers::ReleaseValidation(p_vkInstance->instance);
	delete p_vkDevice;
	delete p_vkInstance;

	// tidy up the shared poiners
	p_world = nullptr;
	p_message = nullptr;
	p_vkGUI = nullptr;
	p_vkMemory = nullptr;
}

void VulkanEngine::DestroyPresentation()
{
	for (uint32_t c = 0; c < m_frameBuffers.size(); ++c) {
		vkDestroyFramebuffer(p_vkDevice->device, m_frameBuffers[c], nullptr);
	}

	vkFreeCommandBuffers(p_vkDevice->device, m_cmdPool, static_cast<uint32_t>(m_cmdBuffers.size()), m_cmdBuffers.data());
}

void VulkanEngine::PrepareNewSwapFrame()
{
	VK_CHECK_RESULT(vkDeviceWaitIdle(p_vkDevice->device));
	
	// destroy frame/command buffers associated with the swapchain
	DestroyPresentation();
	p_vkSwapChain->Destroy();			// destroy the actual swapchain and image views

	// re-initialise the swap chain 
	p_vkSwapChain->Init(p_vkDevice, p_vkInstance->instance, Engine::SCREEN_WIDTH, Engine::SCREEN_HEIGHT);

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

float VulkanEngine::exposureSetting() const
{
	return p_vkGUI->m_guiSettings.exposure;
}

float VulkanEngine::gammaSetting() const
{
	return p_vkGUI->m_guiSettings.gamma;
}

bool VulkanEngine::debugShadowSetting() const
{
	return p_vkGUI->m_guiSettings.debugShadow;
}

// Getter/Setter functions ==================================================================================================================================================================================================================================

VkDevice VulkanEngine::GetDevice() const 
{ 
	return p_vkDevice->device; 
}

VkPhysicalDevice VulkanEngine::GetPhysicalDevice() const
{ 
	return p_vkDevice->physDevice;
}

uint32_t VulkanEngine::GetSurfaceExtentW() 
{ 
	return p_vkSwapChain->surfaceInfo.extent.width; 
}

uint32_t VulkanEngine::GetSurfaceExtentH() 
{ 
	return p_vkSwapChain->surfaceInfo.extent.height; 
}

uint32_t VulkanEngine::GetGraphQueueIndex() const 
{
	return p_vkDevice->queue.graphIndex;
}

VkQueue VulkanEngine::GetGraphQueue() const 
{
	return p_vkDevice->queue.graphQueue; 
}

uint32_t VulkanEngine::GetComputeQueueIndex() const
{ 
	return p_vkDevice->queue.computeIndex;
}

VkQueue VulkanEngine::GetComputeQueue() const 
{ 
	return p_vkDevice->queue.computeQueue;
}

uint32_t VulkanEngine::GetSwapChainImageCount() const 
{ 
	return p_vkSwapChain->imageCount; 
}

VkImageView VulkanEngine::GetImageView(const uint32_t index) const 
{ 
	return p_vkSwapChain->images[index];
}

VkFormat VulkanEngine::GetSurfaceFormat() const 
{ 
	return p_vkSwapChain->surfaceInfo.format.format; 
}

VkRenderPass VulkanEngine::GetFinalRenderPass() const 
{
	return m_renderpass->renderpass; 
}




*/