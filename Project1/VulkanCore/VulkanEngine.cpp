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
#include "Engine/World.h"
#include "utility/file_log.h"


VulkanEngine::VulkanEngine(GLFWwindow *window) :
	VulkanCore(window),
	drawStateChanged(true),
	vk_prepared(false)
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
		}
	}
}

void VulkanEngine::RenderScene(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline)
{
	if (hasModule<VulkanTerrain>()) {
		VkModule<VulkanTerrain>()->GenerateTerrainCmdBuffer(cmdBuffer, set, layout, pipeline);
	}

	if (hasModule<VulkanWater>()) {
		VkModule<VulkanWater>()->GenerateWaterCmdBuffer(cmdBuffer, set, layout, pipeline);
	}

	if (hasModule<VulkanModel>()) {
		VkModule<VulkanModel>()->GenerateModelCmdBuffer(cmdBuffer, set, layout, pipeline); 
	}
}

void VulkanEngine::DrawScene()
{
	// generate BDRF lut for PBR
	VkModule<VulkanPBR>()->GenerateLUTCmdBuffer();

	// generate irradiance and prefilter maps for environment cube
	VkModule<VulkanIBL>()->GenerateIrrMapCmdBuffer();
	VkModule<VulkanIBL>()->GeneratePreFilterCmdBuffer();
	
	// draw into offscreen buffers
	m_offscreenCmdBuffer = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, m_cmdPool);
	
	if (hasModule<VulkanWater>()) {
		VkModule<VulkanWater>()->GenerateOffscreenCmdBuffer(m_offscreenCmdBuffer);
	}

	VkModule<VulkanShadow>()->GenerateShadowCmdBuffer(m_offscreenCmdBuffer);
	VK_CHECK_RESULT(vkEndCommandBuffer(m_offscreenCmdBuffer));

	std::array<VkClearValue, 9> clearValues = {};
	clearValues[0].color = CLEAR_COLOR;			// position
	clearValues[1].color = CLEAR_COLOR;			// normal
	clearValues[2].color = CLEAR_COLOR;			// albedo
	clearValues[3].color = CLEAR_COLOR;			// ao
	clearValues[4].color = CLEAR_COLOR;			// metallic
	clearValues[5].color = CLEAR_COLOR;			// roughness
	clearValues[6].color = CLEAR_COLOR;			// roughness
	clearValues[7].color = CLEAR_COLOR;			// roughness
	clearValues[8].depthStencil = { 1.0f, 0 };

	auto vkDeferred = VkModule<VulkanDeferred>();

	m_cmdBuffers.resize(vkDeferred->m_deferredInfo.frameBuffers.size());

	for (uint32_t c = 0; c < m_cmdBuffers.size(); ++c) {

		m_cmdBuffers[c] = vkUtility->CreateCmdBuffer(vkUtility->VK_PRIMARY, vkUtility->VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, m_cmdPool);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.framebuffer = vkDeferred->m_deferredInfo.frameBuffers[c];
		renderPassInfo.renderPass = vkDeferred->m_deferredInfo.renderPass;
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent.width = m_surface.extent.width;
		renderPassInfo.renderArea.extent.height = m_surface.extent.height;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VkViewport viewport = vkUtility->InitViewPort(m_surface.extent.width, m_surface.extent.height, 0.0f, 1.0f);
		vkCmdSetViewport(m_cmdBuffers[c], 0, 1, &viewport);

		VkRect2D scissor = vkUtility->InitScissor(m_surface.extent.width, m_surface.extent.height, 0, 0);
		vkCmdSetScissor(m_cmdBuffers[c], 0, 1, &scissor);

		vkCmdBeginRenderPass(m_cmdBuffers[c], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// first pass - render the scene into the G buffers
		RenderScene(m_cmdBuffers[c]);

		// second pass - draw scene as a full screen quad
		vkCmdNextSubpass(m_cmdBuffers[c], VK_SUBPASS_CONTENTS_INLINE);
		vkDeferred->GenerateDeferredCmdBuffer(m_cmdBuffers[c]);

		// third pass - draw skybox : this is done so it won't be affected by lighting calculations
		if (hasModule<VulkanSkybox>()) {

			vkCmdNextSubpass(m_cmdBuffers[c], VK_SUBPASS_CONTENTS_INLINE);
			VkModule<VulkanSkybox>()->GenerateSkyboxCmdBuffer(m_cmdBuffers[c]);
		}

		vkCmdEndRenderPass(m_cmdBuffers[c]);
		VK_CHECK_RESULT(vkEndCommandBuffer(m_cmdBuffers[c]));
	}
	
	vk_prepared = true;
}

void VulkanEngine::SubmitFrame()
{
	uint32_t imageIndex = vkUtility->InitRenderFrame();
	auto vkShadow = VkModule<VulkanShadow>();

	VkSubmitInfo submit_info = {};
	VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submit_info.pWaitDstStageMask = &flags;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.waitSemaphoreCount = 1;
	submit_info.signalSemaphoreCount = 1;

	if (hasModule<VulkanWater>()) {
		auto vkWater = VkModule<VulkanWater>();

		// submit compute shaders for spectrum, fft and displacement map computations
		vkWater->SubmitWaterCompute();
	}

	// wait for the offscreen buffer to finish	
	submit_info.pWaitSemaphores = &m_semaphore.image;
	submit_info.pSignalSemaphores = &vkShadow->m_shadowInfo.semaphore;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_offscreenCmdBuffer;
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	submit_info.pWaitSemaphores = &vkShadow->m_shadowInfo.semaphore;
	submit_info.pSignalSemaphores = &m_semaphore.render;
 	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &m_cmdBuffers[imageIndex];
	VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

	vkUtility->SubmitFrame(imageIndex);
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

void VulkanEngine::Update(int acc_time)
{
	for (auto &mod : m_vkModules) {

		mod.second->Update(acc_time);
	}
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
}




