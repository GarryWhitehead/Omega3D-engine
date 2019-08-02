#include "Shadow.h"
#include "ObjectInterface/ComponentTypes.h"
#include "Rendering/RenderCommon.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Utility/logger.h"
#include "VulkanAPI/BufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/VkTextureManager.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Interface.h"

namespace OmegaEngine
{

RenderableShadow::RenderableShadow(std::unique_ptr<ProgramStateManager>& stateManager,
                                   std::unique_ptr<VulkanAPI::Interface>& vkInterface, ShadowComponent& component,
                                   StaticMesh& mesh, PrimitiveMesh& primitive, uint32_t lightCount,
                                   uint32_t lightAlignmentSize, std::unique_ptr<RendererBase>& renderer)
    : RenderableBase(RenderTypes::ShadowMapped)
{
	// fill out the data which will be used for rendering
	instanceData = new ShadowInstance;
	ShadowInstance* shadowInstance = reinterpret_cast<ShadowInstance*>(instanceData);

	// create the sorting key  TODO: actually implement this!
	sortKey = RenderQueue::createSortKey(RenderStage::First, 0, RenderTypes::ShadowMapped);

	queueType = QueueType::Shadow;

	// pointer to the mesh pipeline
	shadowInstance->state =
	    stateManager->createState(vkInterface, renderer, StateType::ShadowMapped, mesh.topology, mesh.type, StateAlpha::Opaque);

	// pointer to the mesh pipeline
	if (mesh.type == StateMesh::Static)
	{
		shadowInstance->vertexBuffer = vkInterface->getBufferManager()->getBuffer("StaticVertices");
	}
	else
	{
		shadowInstance->vertexBuffer = vkInterface->getBufferManager()->getBuffer("SkinnedVertices");
	}

	// index into the main buffer - this is the vertex offset plus the offset into the actual memory segment
	shadowInstance->vertexOffset = mesh.vertexBufferOffset;
	shadowInstance->indexOffset = mesh.indexBufferOffset;
	shadowInstance->indexBuffer = vkInterface->getBufferManager()->getBuffer("Indices");
	shadowInstance->indexCount = primitive.indexCount;

	shadowInstance->lightCount = lightCount;
	shadowInstance->lightAlignmentSize = lightAlignmentSize;

	shadowInstance->biasClamp = component.biasClamp;
	shadowInstance->biasConstant = component.biasConstant;
	shadowInstance->biasSlope = component.biasSlope;
}

RenderableShadow::~RenderableShadow()
{
}

void RenderableShadow::createShadowPipeline(std::unique_ptr<VulkanAPI::Interface>& vkInterface,
                                            std::unique_ptr<RendererBase>& renderer,
                                            std::unique_ptr<ProgramState>& state, StateId::StateFlags& flags)
{
	if (!state->shader.add(vkInterface->getDevice(), "shadow/mapped-vert.spv", VulkanAPI::StageType::Vertex))
	{
		LOGGER_ERROR("Unable to create static shadow shaders.");
	}

	// get pipeline layout and vertedx attributes by reflection of shader
	state->shader.imageReflection(state->descriptorLayout, state->imageLayout);
	state->shader.bufferReflection(state->descriptorLayout, state->bufferLayout);
	state->descriptorLayout.create(vkInterface->getDevice());
	state->descriptorSet.init(vkInterface->getDevice(), state->descriptorLayout);

	// sort out the descriptor sets - buffers
	for (auto& layout : state->bufferLayout.layouts)
	{
		// the shader must use these identifying names for uniform buffers -
		if (layout.name == "Dynamic_Ubo")
		{
			vkInterface->getBufferManager()->enqueueDescrUpdate("LightDynamic", &state->descriptorSet, layout.set,
			                                                    layout.binding, layout.type);
		}
	}

	state->shader.pipelineLayoutReflect(state->pipelineLayout);
	state->pipelineLayout.create(vkInterface->getDevice(), state->descriptorLayout.getLayout());

	// create the graphics pipeline
	state->shader.pipelineReflection(state->pipeline);

	state->pipeline.setDepthState(VK_TRUE, VK_TRUE);
	state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
	state->pipeline.setRasterFrontFace(vk::FrontFace::eClockwise);
	state->pipeline.setTopology(flags.topology);
	state->pipeline.addDynamicState(vk::DynamicState::eDepthBias);
	state->pipeline.create(vkInterface->getDevice(), renderer->getShadowPass(), state->shader, state->pipelineLayout,
	                       VulkanAPI::PipelineType::Graphics);
}

void RenderableShadow::createShadowPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image,
                                        vk::Device& device, vk::PhysicalDevice& gpu, const vk::Format format,
                                        const uint32_t width, const uint32_t height)
{
	// create empty image into which the depth will be drawn
	image.createEmptyImage(format, width, height, 1,
	                       vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled);

	// renderpass
	renderpass.init(device);
	renderpass.addAttachment(format, VulkanAPI::FinalLayoutType::Auto);
	renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencilSubpassTop);
	renderpass.addSubpassDependency(VulkanAPI::DependencyTemplate::DepthStencilSubpassBottom);
	renderpass.prepareRenderPass();

	// framebuffer
	renderpass.prepareFramebuffer(image.getImageView(), width, height, 1);
}

void RenderableShadow::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instance)
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
		cmdBuffer.bindDynamicDescriptors(state->pipelineLayout, state->descriptorSet, VulkanAPI::PipelineType::Graphics,
		                                 dynamicBufferOffset);
		cmdBuffer.drawIndexed(instanceData->indexCount);
	}
}
}    // namespace OmegaEngine
