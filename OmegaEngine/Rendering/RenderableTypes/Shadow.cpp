#include "Shadow.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/BufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderCommon.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include "Utility/logger.h"
#include "ObjectInterface/ComponentTypes.h"

namespace OmegaEngine
{

	RenderableShadow::RenderableShadow(RenderInterface* renderInterface, 
										ShadowComponent& component, 
										RenderableMesh::MeshInstance* meshInstance,
										uint32_t lightCount,
										uint32_t lightAlignmentSize) :
		RenderableBase(RenderTypes::Skybox)
	{
		// fill out the data which will be used for rendering
		instanceData = new ShadowInstance;
		ShadowInstance* shadowInstance = reinterpret_cast<ShadowInstance*>(instanceData);
		
		// create the sorting key  TODO: actually implement this!
		sortKey = RenderQueue::createSortKey(RenderStage::First, 0, RenderTypes::ShadowMapped);

		queueType = QueueType::Shadow;

		// pointer to the mesh pipeline
		shadowInstance->state = renderInterface->getRenderPipeline(RenderTypes::ShadowMapped).get();

		// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
		shadowInstance->vertexOffset = meshInstance->vertexOffset;
		shadowInstance->indexOffset = meshInstance->indexOffset;
		shadowInstance->vertexBuffer = meshInstance->vertexBuffer;
		shadowInstance->indexBuffer = meshInstance->indexBuffer;
		shadowInstance->indexCount = meshInstance->indexPrimitiveCount;
		
		shadowInstance->lightCount = lightCount;
		shadowInstance->lightAlignmentSize = lightAlignmentSize;

		shadowInstance->biasClamp = component.biasClamp;
		shadowInstance->biasConstant = component.biasConstant;
		shadowInstance->biasSlope = component.biasSlope;
	}

	RenderableShadow::~RenderableShadow()
	{
	}

	void RenderableShadow::createShadowPipeline(vk::Device& device,
		std::unique_ptr<RendererBase>& renderer,
		std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
		std::unique_ptr<ProgramState>& state)
	{
		if (!state->shader.add(device, "shadow/mapped-vert.spv", VulkanAPI::StageType::Vertex))
		{
				LOGGER_ERROR("Unable to create static shadow shaders.");
		}
		
		// get pipeline layout and vertedx attributes by reflection of shader
		state->shader.imageReflection(state->descriptorLayout, state->imageLayout);
		state->shader.bufferReflection(state->descriptorLayout, state->bufferLayout);
		state->descriptorLayout.create(device);
		state->descriptorSet.init(device, state->descriptorLayout);

		// sort out the descriptor sets - buffers
		for (auto& layout : state->bufferLayout.layouts) 
		{
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "Dynamic_Ubo")
			{
				bufferManager->enqueueDescrUpdate("LightDynamic", &state->descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		state->shader.pipelineLayoutReflect(state->pipelineLayout);
		state->pipelineLayout.create(device, state->descriptorLayout.getLayout());

		// create the graphics pipeline
		state->shader.pipelineReflection(state->pipeline);

		state->pipeline.setDepthState(VK_TRUE, VK_TRUE);
		state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
		state->pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state->pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state->pipeline.addDynamicState(vk::DynamicState::eDepthBias);
		state->pipeline.create(device, renderer->getShadowPass(), state->shader, state->pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableShadow::createShadowPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, 
		vk::Device& device, vk::PhysicalDevice& gpu, const vk::Format format, const uint32_t width, const uint32_t height)
	{
		// create empty image into which the depth will be drawn
		image.createEmptyImage(format, width, height,
			1, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);

		// renderpass
		renderpass.init(device);
		renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, format);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencilSubpassTop);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencilSubpassBottom);
		renderpass.prepareRenderPass();

		// framebuffer
		renderpass.prepareFramebuffer(image.getImageView(), width, height, 1);
	}

	void RenderableShadow::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
								void* instance)
	{
		ShadowInstance* instanceData = (ShadowInstance*)instance;

		ProgramState* state = instanceData->state;

		cmdBuffer.setViewport();
		cmdBuffer.setScissor();
		cmdBuffer.setDepthBias(instanceData->biasConstant, instanceData->biasClamp, instanceData->biasSlope);
		cmdBuffer.bindPipeline(state->pipeline);
		
		vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
		cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
		cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer, instanceData->indexBuffer.offset);

		// we need to render the object from each light sources point of view
		for (uint32_t i = 0; i < instanceData->lightCount; ++i)
		{
			uint32_t dynamicBufferOffset = i * instanceData->lightAlignmentSize;
			cmdBuffer.bindDynamicDescriptors(state->pipelineLayout, state->descriptorSet, VulkanAPI::PipelineType::Graphics, dynamicBufferOffset);
			cmdBuffer.drawIndexed(instanceData->indexCount);
		}
	}
}
