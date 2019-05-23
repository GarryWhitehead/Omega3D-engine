#include "Shadow.h"
#include "Vulkan/Shader.h"
#include "Vulkan/BufferManager.h"
#include "Vulkan/CommandBuffer.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderCommon.h"
#include "Vulkan/DataTypes/Texture.h"
#include "Utility/logger.h"
#include "Objects/ObjectTypes.h"

namespace OmegaEngine
{

	RenderableShadow::RenderableShadow(RenderInterface* renderInterface, ShadowComponent& component, RenderableMesh::MeshInstance* meshInstance) :
		RenderableBase(RenderTypes::Skybox)
	{
		// fill out the data which will be used for rendering
		instanceData = new ShadowInstance;
		ShadowInstance* shadowInstance = reinterpret_cast<ShadowInstance*>(instanceData);
		
		// create the sorting key  TODO: actually implement this!
		sort_key = RenderQueue::createSortKey(RenderStage::First, 0, RenderTypes::ShadowStatic);

		queueType = QueueType::Shadow;

		// pointer to the mesh pipeline
		if (meshInstance->type == MeshManager::MeshType::Static) {
			shadowInstance->state = renderInterface->getRenderPipeline(RenderTypes::ShadowStatic).get();
		}
		else {
			shadowInstance->state = renderInterface->getRenderPipeline(RenderTypes::ShadowDynamic).get();
		}

		// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
		shadowInstance->meshType = meshInstance->type;
		shadowInstance->vertexOffset = meshInstance->vertexOffset;
		shadowInstance->indexOffset = meshInstance->indexOffset;
		shadowInstance->vertexBuffer = meshInstance->vertexBuffer;
		shadowInstance->indexBuffer = meshInstance->indexBuffer;
		shadowInstance->transformDynamicOffset = meshInstance->transformDynamicOffset;
		shadowInstance->skinnedDynamicOffset = meshInstance->skinnedDynamicOffset;

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
		std::unique_ptr<ProgramState>& state,
		MeshManager::MeshType type)
	{
		// load shaders - using the same shaders as the mesh, as we want to draw the vertices data, but aren't interested in colour information just depth
		if (type == MeshManager::MeshType::Static) {
			if (!state->shader.add(device, "model/model-vert.spv", VulkanAPI::StageType::Vertex)) {
				LOGGER_ERROR("Unable to create static shadow shaders.");
			}
		}
		else if (type == MeshManager::MeshType::Skinned) {
			if (!state->shader.add(device, "model/model_skinned-vert.spv", VulkanAPI::StageType::Vertex)) {
				LOGGER_ERROR("Unable to create skinned shadow shaders.");
			}
		}

		// get pipeline layout and vertedx attributes by reflection of shader
		state->shader.imageReflection(state->descriptorLayout, state->imageLayout);
		state->shader.bufferReflection(state->descriptorLayout, state->bufferLayout);
		state->descriptorLayout.create(device);
		state->descriptorSet.init(device, state->descriptorLayout);

		// sort out the descriptor sets - buffers
		for (auto& layout : state->bufferLayout) {
			
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "CameraUbo") {
				bufferManager->enqueueDescrUpdate("Camera", &state->descriptorSet, layout.set, layout.binding, layout.type);
			}
			else if (layout.name == "Dynamic_StaticMeshUbo") {
				bufferManager->enqueueDescrUpdate("Transform", &state->descriptorSet, layout.set, layout.binding, layout.type);
			}
			else if (layout.name == "Dynamic_SkinnedUbo") {
				bufferManager->enqueueDescrUpdate("SkinnedTransform", &state->descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		state->shader.pipelineLayoutReflect(state->pipelineLayout);
		state->pipelineLayout.create(device, state->descriptorLayout.getLayout());

		// create the graphics pipeline
		state->shader.pipelineReflection(state->pipeline);

		state->pipeline.setDepthState(VK_TRUE, VK_FALSE);
		state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eFront);
		state->pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
		state->pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state->pipeline.addDynamicState(vk::DynamicState::eDepthBias);
		state->pipeline.create(device, renderer->getShadowPass(), state->shader, state->pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableShadow::createShadowPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, 
		vk::Device& device, vk::PhysicalDevice& gpu, const vk::Format format, const uint32_t width, const uint32_t height)
	{
		
		// renderpass
		renderpass.init(device);
		renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, format);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencilSubpassTop);
		renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencilSubpassBottom);
		renderpass.prepareRenderPass();

		// framebuffer
		image.createEmptyImage(format, width, height,
			1, vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);

		renderpass.prepareFramebuffer(image.getImageView(), width, height, 1);
	}

	void RenderableShadow::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
								void* instance)
	{
		ShadowInstance* instanceData = (ShadowInstance*)instance;

		ProgramState* state = instanceData->state;

		std::vector<uint32_t> dynamicOffsets{ instanceData->transformDynamicOffset };
		if (instanceData->meshType == MeshManager::MeshType::Skinned) {
			dynamicOffsets.push_back(instanceData->skinnedDynamicOffset);
		}

		cmdBuffer.setViewport();
		cmdBuffer.setScissor();
		cmdBuffer.setDepthBias(instanceData->biasConstant, instanceData->biasClamp, instanceData->biasSlope);
		cmdBuffer.bindPipeline(state->pipeline);
		cmdBuffer.bindDynamicDescriptors(state->pipelineLayout, state->descriptorSet, VulkanAPI::PipelineType::Graphics, dynamicOffsets);

		vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
		cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
		cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer, instanceData->indexBuffer.offset);
		cmdBuffer.drawIndexed(instanceData->indexCount);
	}
}
