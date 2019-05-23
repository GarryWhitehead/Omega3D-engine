#include "DeferredRenderer.h"
#include "Utility/logger.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderCommon.h"
#include "Rendering/RenderableTypes/Shadow.h"
#include "Rendering/RenderableTypes/Skybox.h"
#include "Engine/Omega_Global.h"
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "Managers/ComponentInterface.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/Sampler.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/Queue.h"
#include "Vulkan/SemaphoreManager.h"
#include "Vulkan/Swapchain.h"
#include "PostProcess/PostProcessInterface.h"
#include "Vulkan/Device.h"

namespace OmegaEngine
{
	
	DeferredRenderer::DeferredRenderer(vk::Device& dev, 
										vk::PhysicalDevice& physical, 
										std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager, 
										std::unique_ptr<VulkanAPI::BufferManager>& bufferManager, 
										VulkanAPI::Swapchain& swapchain, RenderConfig& _renderConfig) :
		device(dev),
		gpu(physical),
		renderConfig(_renderConfig),
		RendererBase(RendererType::Deferred)
	{
		deferredCmdBufferHandle = cmdBufferManager->createInstance();
		objectCmdBufferHandle = cmdBufferManager->createInstance();
		forwardCmdBufferHandle = cmdBufferManager->createInstance();

		// set up the deferred passes and shadow stuff
		// 1. render all objects into the gbuffer pass - seperate images for pos, base-colour, normal, pbr and emissive
		createGbufferPass();

		// 2. render the objects again but this time into a depth buffer for shadows
		shadowImage.init(device, gpu);
		RenderableShadow::createShadowPass(shadowRenderpass, shadowImage, device, gpu, 
			renderConfig.shadowFormat, renderConfig.shadowWidth, renderConfig.shadowHeight);

		forwardOffscreenImage.init(device, gpu);
		forwardOffscreenDepthImage.init(device, gpu);
		RenderableSkybox::createSkyboxPass(forwardRenderpass, forwardOffscreenImage, forwardOffscreenDepthImage, 
			device, gpu, renderConfig.deferred.offscreenWidth, renderConfig.deferred.offscreenHeight);

		// 3. The image attachments are used in the deffered pass to calcuate pixel colour based on lighting calculations
		createDeferredPass(bufferManager, swapchain);
	}


	DeferredRenderer::~DeferredRenderer()
	{
	}

	void DeferredRenderer::createGbufferPass()
	{
		// a list of the formats required for each buffer
		vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);

		firstRenderpass.init(device);
		firstRenderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16B16A16Sfloat);		// position
		firstRenderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR8G8B8A8Unorm);			// colour
		firstRenderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16B16A16Sfloat);		// normal
		firstRenderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16Sfloat);				// pbr
		firstRenderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, vk::Format::eR16G16B16A16Sfloat);		// emissive
		firstRenderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depthFormat);					// depth
		firstRenderpass.prepareRenderPass();

		const uint8_t attachmentCount = 6;

		// init textures
		for (auto& texture : gBufferImages)
		{
			texture.init(device, gpu);
		}

		// create a empty texture for each state - these will be filled by the shader
		for (uint8_t i = 0; i < attachmentCount - 1; ++i) 
		{
			gBufferImages[i].createEmptyImage(firstRenderpass.get_attachment_format(i), 
				renderConfig.deferred.gBufferWidth, renderConfig.deferred.gBufferHeight, 
				1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);
		}

		// and the depth g-buffer
		gBufferImages[attachmentCount - 1].createEmptyImage(depthFormat, 
			renderConfig.deferred.gBufferWidth, renderConfig.deferred.gBufferHeight, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment| vk::ImageUsageFlagBits::eTransferSrc);

		// tie the image-views to the frame buffer
		std::vector<vk::ImageView> imageViews(attachmentCount);

		for (uint8_t i = 0; i < attachmentCount; ++i) 
		{
			imageViews[i] = gBufferImages[i].getImageView();
		}

		firstRenderpass.prepareFramebuffer(static_cast<uint32_t>(imageViews.size()), imageViews.data(), 
			renderConfig.deferred.gBufferWidth, renderConfig.deferred.gBufferHeight);
	}


	void DeferredRenderer::createDeferredPass(std::unique_ptr<VulkanAPI::BufferManager>& bufferManager, VulkanAPI::Swapchain& swapchain)
	{
		// load the shaders and carry out reflection to create the pipeline and descriptor layouts
		if (!state.shader.add(device, "renderer/deferred/deferred-vert.spv", VulkanAPI::StageType::Vertex, "renderer/deferred/deferred-frag.spv", VulkanAPI::StageType::Fragment)) 
		{
			LOGGER_ERROR("Unable to load deferred renderer shaders.");
			throw std::runtime_error("Error whilst trying to open deferred shader file.");
		}

		// create the descriptors and pipeline layout through shader reflection
		state.shader.bufferReflection (state.descriptorLayout, state.bufferLayout);
		state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
		state.shader.pipelineLayoutReflect(state.pipelineLayout);
		state.shader.pipelineReflection(state.pipeline);

		state.descriptorLayout.create(device);
		state.descriptorSet.init(device, state.descriptorLayout);

		// not completely automated! We still need to manually adjust the set numbers for each type
		const uint8_t DeferredSet = 1;
		const uint8_t EnvironmentSet = 2;

		for (uint8_t i = 0; i < state.imageLayout[DeferredSet].size(); ++i) 
		{
			state.descriptorSet.writeSet(state.imageLayout[DeferredSet][i], gBufferImages[i].getImageView());
		}
		
		for (auto& layout : state.bufferLayout) 
		{
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") 
			{
				bufferManager->enqueueDescrUpdate("Camera", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
			if (layout.name == "LightUbo") 
			{
				bufferManager->enqueueDescrUpdate("Light", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		// and finally create the pipeline
		// first finish of the pipeline layout....
		state.pipelineLayout.create(device, state.descriptorLayout.getLayout());

		state.pipeline.setDepthState(VK_TRUE, VK_TRUE);
		state.pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
		
		if (renderConfig.general.useSkybox) 
		{
			state.pipeline.addColourAttachment(VK_FALSE, forwardRenderpass);
			state.pipeline.create(device, forwardRenderpass, state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);
		}
		else 
		{
			// render to the swapchain presentation 
			state.pipeline.addColourAttachment(VK_FALSE, swapchain.getRenderpass());
			state.pipeline.create(device, swapchain.getRenderpass(), state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);
		}
	}

	void DeferredRenderer::renderDeferredPass(std::unique_ptr<VulkanAPI::CommandBufferManager>& cmdBufferManager, VulkanAPI::Swapchain& swapchain)
	{
		cmdBufferManager->beginNewFame(deferredCmdBufferHandle);
		auto& cmdBuffer = cmdBufferManager->getCmdBuffer(deferredCmdBufferHandle);

		cmdBuffer->createPrimary();
		vk::RenderPassBeginInfo beginInfo = forwardRenderpass.getBeginInfo(vk::ClearColorValue(renderConfig.general.backgroundColour));
		cmdBuffer->beginRenderpass(beginInfo);
		
		// viewport and scissor
		cmdBuffer->setViewport();
		cmdBuffer->setScissor();

		// bind everything required to draw
		cmdBuffer->bindPipeline(state.pipeline);
		cmdBuffer->bindDescriptors(state.pipelineLayout, state.descriptorSet, VulkanAPI::PipelineType::Graphics);
		cmdBuffer->bindPushBlock(state.pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(RenderConfig::IBLInfo), &renderConfig.ibl);

		// render full screen quad to screen
		cmdBuffer->drawQuad();

		// end this pass and cmd buffer
		cmdBuffer->endRenderpass();
		cmdBuffer->end();
	
	}

	void DeferredRenderer::render(std::unique_ptr<VulkanAPI::Interface>& vkInterface, SceneType sceneType, std::unique_ptr<RenderQueue>& renderQueue)
	{
		auto& cmdBufferManager = vkInterface->getCmdBufferManager();

		if (sceneType == SceneType::Dynamic || (sceneType == SceneType::Static && !cmdBufferManager->isRecorded(deferredCmdBufferHandle))) 
		{
			cmdBufferManager->beginNewFame(objectCmdBufferHandle);

			// draw all objects into the shadow offscreen depth buffer 
			Rendering::renderObjects(renderQueue, shadowRenderpass, cmdBufferManager->getCmdBuffer(objectCmdBufferHandle), QueueType::Shadow, renderConfig);

			// generate the g-buffers by drawing the components into the offscreen frame-buffers
			Rendering::renderObjects(renderQueue, firstRenderpass, cmdBufferManager->getCmdBuffer(objectCmdBufferHandle), QueueType::Opaque, renderConfig);

			// render the deffered pass - lights, shadow and IBL contribution
			renderDeferredPass(cmdBufferManager, vkInterface->getSwapchain());

			// skybox is done in a separate forward pass, with the depth buffer blitted from the deferred pass
			if (renderConfig.general.useSkybox) 
			{
				cmdBufferManager->beginNewFame(forwardCmdBufferHandle);

				// we will use the depth buffer from the first pass - this is used to only draw the skybox where there is no pixels
				forwardOffscreenDepthImage.getImage().blit(gBufferImages[5].getImage(), vkInterface->getGraphicsQueue());
				Rendering::renderObjects(renderQueue, forwardRenderpass, cmdBufferManager->getCmdBuffer(forwardCmdBufferHandle), QueueType::Forward, renderConfig);
			}

			postProcessInterface->render();
		}

		// finally send to the swap-chain presentation
		cmdBufferManager->submitFrame(vkInterface->getSwapchain());
	}
}
