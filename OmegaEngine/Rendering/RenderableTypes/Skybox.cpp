#include "Skybox.h"
#include "Vulkan/Shader.h"
#include "Vulkan/VkTextureManager.h"
#include "Vulkan/CommandBuffer.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/StockModels.h"
#include "Objects/ObjectTypes.h"
#include "Rendering/RenderCommon.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	RenderableSkybox::RenderableSkybox(RenderInterface* renderInterface, SkyboxComponent& component, std::unique_ptr<VulkanAPI::BufferManager>& bufferManager) :
		RenderableBase(RenderTypes::Skybox)
	{
		// fill out the data which will be used for rendering
		instanceData = new SkyboxInstance;
		SkyboxInstance* skyboxInstance = reinterpret_cast<SkyboxInstance*>(instanceData);
		
		// create the sorting key for this mesh
		sortKey = RenderQueue::createSortKey(RenderStage::First, 0, RenderTypes::Skybox);

		queueType = QueueType::Forward;

		skyboxInstance->state = renderInterface->getRenderPipeline(RenderTypes::Skybox).get();
		skyboxInstance->blurFactor = component.blurFactor;
		
		skyboxInstance->vertexBuffer = bufferManager->getBuffer("CubeModelVertices");
		skyboxInstance->indexBuffer = bufferManager->getBuffer("CubeModelIndices");
		skyboxInstance->indexCount = RenderUtil::CubeModel::indicesSize;
	}


	RenderableSkybox::~RenderableSkybox()
	{
	}

	void RenderableSkybox::createSkyboxPipeline(vk::Device& device,
		std::unique_ptr<RendererBase>& renderer,
		std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
		std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
		std::unique_ptr<ProgramState>& state)
	{
		// load shaders
		if (!state.shader.add(device, "env/Skybox/skybox-vert.spv", VulkanAPI::StageType::Vertex, "env/Skybox/skybox-frag.spv", VulkanAPI::StageType::Fragment)) {
			LOGGER_ERROR("Unable to create skybox shaders.");
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state.shader.imageReflection(state.descriptorLayout, state.imageLayout);
		state.shader.bufferReflection(state.descriptorLayout, state.bufferLayout);
		state.descriptorLayout.create(device);
		state.descriptorSet.init(device, state.descriptorLayout);

		// sort out the descriptor sets - buffers
		for (auto& layout : state.bufferLayout) {

			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				bufferManager->enqueueDescrUpdate("Camera", &state.descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		// sort out the descriptor sets - images
		for (auto& layout : state.imageLayout) {

			for (auto& image : layout.second)
			{
				// the shader must use these identifying names for uniform buffers -
				if (image.name == "SkyboxSampler") {
					textureManager->enqueueDescrUpdate("Skybox", &state.descriptorSet, &image.sampler, image.set, image.binding);
				}
			}
		}

		state.shader.pipelineLayoutReflect(state.pipelineLayout);
		state.pipelineLayout.create(device, state.descriptorLayout.getLayout());

		// create the graphics pipeline
		state.shader.pipelineReflection(state.pipeline);

		state.pipeline.setDepthState(VK_TRUE, VK_FALSE);
		state.pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
		state.pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state.pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state.pipeline.addColourAttachment(VK_FALSE, renderer->getForwardPass());
		state.pipeline.create(device, renderer->getForwardPass(), state.shader, state.pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableSkybox::createSkyboxPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, VulkanAPI::Texture& depthImage, 
		vk::Device& device, vk::PhysicalDevice& gpu, const uint32_t width, const uint32_t height)
	{
		vk::Format format = vk::Format::eR16G16B16A16Sfloat;
		vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);

		// now create the renderpasses and frame buffers
		renderpass.init(device);
		renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, format);
		renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depthFormat);
		renderpass.prepareRenderPass();

		// colour
		image.createEmptyImage(format, width, height,
			1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc);

		// depth - this will be blitted with the depth buffer from the previous pass
		depthImage.createEmptyImage(depthFormat,
			width, height, 1, vk::ImageUsageFlagBits::eDepthStencilAttachment);

		// frame buffer prep
		std::vector<vk::ImageView> imageViews{ image.getImageView() , depthImage.getImageView() };
		renderpass.prepareFramebuffer(static_cast<uint32_t>(imageViews.size()), imageViews.data(), width, height, 1);
	}

	void RenderableSkybox::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
								void* instance)
	{
		SkyboxInstance* instanceData = (SkyboxInstance*)instance;

		ProgramState* state = instanceData->state;

		cmdBuffer.setViewport();
		cmdBuffer.setScissor();
		cmdBuffer.bindPipeline(state.pipeline);
		cmdBuffer.bindDescriptors(state.pipelineLayout, state.descriptorSet, VulkanAPI::PipelineType::Graphics);
		cmdBuffer.bindPushBlock(state.pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(float), &instanceData->blurFactor);

		vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
		cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
		cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer, instanceData->indexBuffer.offset);
		cmdBuffer.drawIndexed(instanceData->indexCount);
	}
}
