#include "Renderer.h"

#include "Core/engine.h"

#include "Components/CameraManager.h"
#include "Components/LightManager.h"

#include "PostProcess/PostProcessInterface.h"

#include "Rendering/IblInterface.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/LightingPass.h"

#include "Utility/logger.h"

namespace OmegaEngine
{

Renderer::Renderer(Engine& eng, Scene& scene, VulkanAPI::Swapchain& swapchain) :
	engine(eng), 
	scene(scene), 
	swapchain(swapchain),
    context(eng.getContext())
{
	// setup ibl if required
	ibl.prepare();
}

Renderer::~Renderer()
{
}

void Renderer::prepare()
{
	// TODO: At the moment only a deffered renderer is supported. Maybe add a
    // forward renderer as well
    for (const RenderStage& stage : deferredStages)
    {
        switch (stage)
        {
            case RenderStage::GBufferFill:
                rStages.emplace_back(std::make_unique<GBufferFillPass>());
                break;
            case RenderStage::LightingPass:
                rStages.emplace_back(std::make_unique<LightingPass>());
                break;
        }
    }
}

void DeferredRenderer::createDeferredPass()
{
	deferredImage.init(device, gpu);
	deferredRenderPass.init(device);

	vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);
	deferredRenderPass.addAttachment(renderConfig.deferred.deferredFormat, VulkanAPI::FinalLayoutType::Auto);
	deferredRenderPass.addAttachment(depthFormat, VulkanAPI::FinalLayoutType::Auto);
	deferredRenderPass.prepareRenderPass();

	// colour
	deferredImage.createEmptyImage(renderConfig.deferred.deferredFormat, renderConfig.deferred.deferredWidth,
	                               renderConfig.deferred.deferredHeight, 1,
	                               vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled |
	                                   vk::ImageUsageFlagBits::eTransferSrc);

	// frame buffer prep
	std::vector<vk::ImageView> imageViews{ deferredImage.getImageView(),
		                                   gBufferImages[(int)gBufferImageIndex::Depth].getImageView() };
	deferredRenderPass.prepareFramebuffer(static_cast<uint32_t>(imageViews.size()), imageViews.data(),
	                                      renderConfig.deferred.deferredWidth, renderConfig.deferred.deferredHeight, 1);
}




void DeferredRenderer::createDeferredPipeline(std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
                                              VulkanAPI::Swapchain& swapchain)
{
	
	

	// create the descriptors and pipeline layout through shader reflection
	state.shader.bufferReflection(state.descriptorLayout, state.bufferLayout);
	state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
	state.shader.pipelineLayoutReflect(state.pipelineLayout);
	state.shader.pipelineReflection(state.pipeline);

	state.descriptorLayout.create(device);
	state.descriptorSet.init(device, state.descriptorLayout);

	// not completely automated! We still need to manually adjust the set numbers for each type
	const uint8_t deferredSet = 1;
	const uint8_t iblSet = 2;

	for (auto& layout : state.imageLayout.layouts)
	{
		if (layout.name == "shadowSampler")
		{
			state.descriptorSet.writeSet(state.imageLayout.find(deferredSet, layout.binding).value(),
			                             shadowImage.getImageView());
		}
		else if (layout.set == deferredSet)
		{
			for (auto& gbufferLayout : gbufferShaderLayout)
			{
				if (gbufferLayout.first == layout.name)
				{
					state.descriptorSet.writeSet(state.imageLayout.find(deferredSet, layout.binding).value(),
					                             gBufferImages[(int)gbufferLayout.second].getImageView());
				}
			}
		}
		else if (layout.set == iblSet)
		{
			if (layout.name == "brdfLutSampler")
			{
				state.descriptorSet.writeSet(state.imageLayout.find(iblSet, layout.binding).value(),
				                             iblInterface->getBrdfImageView());
			}
			else if (layout.name == "irradianceSampler")
			{
				state.descriptorSet.writeSet(state.imageLayout.find(iblSet, layout.binding).value(),
				                             iblInterface->getIrradianceMapImageView());
			}
			else if (layout.name == "prefilterSampler")
			{
				state.descriptorSet.writeSet(state.imageLayout.find(iblSet, layout.binding).value(),
				                             iblInterface->getSpecularMapImageView());
			}
		}
	}

	for (auto& layout : state.bufferLayout.layouts)
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

	state.pipeline.setDepthState(VK_TRUE, VK_FALSE);
	state.pipeline.setTopology(StateTopology::List);
	state.pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
	state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
	state.pipeline.addColourAttachment(VK_FALSE, deferredRenderPass);
	state.pipeline.create(device, deferredRenderPass, state.shader, state.pipelineLayout,
	                      VulkanAPI::PipelineType::Graphics);
}

void Renderer::renderDeferredPass(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer)
{
	
}

void Renderer::render(std::unique_ptr<VulkanAPI::Interface>& vkInterface, SceneType sceneType,
                              std::unique_ptr<RenderQueue>& renderQueue)
{
	auto& cmdBufferManager = vkInterface->getCmdBufferManager();

	if (sceneType == SceneType::Dynamic ||
	    (sceneType == SceneType::Static && !cmdBufferManager->isRecorded(deferredCmdBufferHandle)))
	{
		auto& shadowCmdBuffer = cmdBufferManager->beginNewFame(shadowCmdBufferHandle);

		// if this is the first render call, then determine whether the ibl maps need generating
		if (!iblInterface->isReady())
		{
			iblInterface->renderMaps(*vkInterface);
		}

		// draw all objects into the shadow offscreen depth buffer
		Rendering::renderObjects(renderQueue, shadowRenderpass, shadowCmdBuffer, QueueType::Shadow, renderConfig, true);
		shadowCmdBuffer->end();

		auto& deferredCmdBuffer = cmdBufferManager->beginNewFame(deferredCmdBufferHandle);

		// generate the g-buffers by drawing the components into the offscreen frame-buffers
		Rendering::renderObjects(renderQueue, firstRenderpass, deferredCmdBuffer, QueueType::Opaque, renderConfig,
		                         true);

		// render the deffered pass - lights, shadow and IBL contribution
		renderDeferredPass(deferredCmdBuffer);
		deferredCmdBuffer->end();

		// init cmd buffers for all forward passes
		auto& forwardCmdBuffer = cmdBufferManager->beginNewFame(forwardCmdBufferHandle);

		// draw the skybox last using the stencil from the gbuffer pass to only draw where there is no geometry
		if (renderConfig.general.useSkybox)
		{
			Rendering::renderObjects(renderQueue, forwardRenderpass, forwardCmdBuffer, QueueType::Forward, renderConfig,
			                         false);
		}

		postProcessInterface->render(renderConfig);
		forwardCmdBuffer->end();
	}

	// and finally render to the presentation surface the final composition with final effects added - fog, etc.
	presentPass->render(*vkInterface, renderConfig);

	// finally send to the swap-chain presentation
	cmdBufferManager->submitFrame(vkInterface->getSwapchain());
}
}    // namespace OmegaEngine
