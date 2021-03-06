#include "Skybox.h"
#include "Engine/Omega_Global.h"
#include "Managers/EventManager.h"
#include "ObjectInterface/ComponentTypes.h"
#include "Rendering/RenderCommon.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Utility/logger.h"
#include "VulkanAPI/BufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkTextureManager.h"

namespace OmegaEngine
{

RenderableSkybox::RenderableSkybox(std::unique_ptr<ProgramStateManager>& stateManager, SkyboxComponent& component,
                                   std::unique_ptr<VulkanAPI::Interface>& vkInterface, std::unique_ptr<RendererBase>& renderer)
    : RenderableBase(RenderTypes::Skybox)
{
	// upload the indices and verices to the gpu. This will happen per skybox
	generateBuffers();

	// fill out the data which will be used for rendering
	instanceData = new SkyboxInstance;
	SkyboxInstance* skyboxInstance = reinterpret_cast<SkyboxInstance*>(instanceData);

	// create the sorting key for this mesh
	sortKey = RenderQueue::createSortKey(RenderStage::First, 0, RenderTypes::Skybox);

	queueType = QueueType::Forward;

	skyboxInstance->state = stateManager->createState(vkInterface, renderer, StateType::Skybox, StateTopology::List,
	                                                  StateMesh::Static, StateAlpha::Opaque);
	skyboxInstance->blurFactor = component.blurFactor;

	skyboxInstance->vertexBuffer = vkInterface->getBufferManager()->getBuffer("CubeModelVertices");
	skyboxInstance->indexBuffer = vkInterface->getBufferManager()->getBuffer("CubeModelIndices");
	skyboxInstance->indexCount = RenderableSkybox::indicesSize;
}

RenderableSkybox::~RenderableSkybox()
{
}

void RenderableSkybox::generateBuffers()
{
	// cube vertices
	static std::array<OEMaths::vec3f, verticesSize> vertices{

		OEMaths::vec3f{ -1.0f, -1.0f, 1.0f },  OEMaths::vec3f{ 1.0f, -1.0f, 1.0f },
		OEMaths::vec3f{ 1.0f, 1.0f, 1.0f },    OEMaths::vec3f{ -1.0f, 1.0f, 1.0f },
		OEMaths::vec3f{ -1.0f, -1.0f, -1.0f }, OEMaths::vec3f{ 1.0f, -1.0f, -1.0f },
		OEMaths::vec3f{ 1.0f, 1.0f, -1.0f },   OEMaths::vec3f{ -1.0f, 1.0f, -1.0f }
	};

	// cube indices
	static std::array<uint32_t, indicesSize> indices{ // front
		                                              0, 1, 2, 2, 3, 0,
		                                              // right side
		                                              1, 5, 6, 6, 2, 1,
		                                              // back
		                                              7, 6, 5, 5, 4, 7,
		                                              // left side
		                                              4, 0, 3, 3, 7, 4,
		                                              // bottom
		                                              4, 5, 1, 1, 0, 4,
		                                              // top
		                                              3, 2, 6, 6, 7, 3
	};

	// vertex data
	VulkanAPI::BufferUpdateEvent vertexEvent{ "CubeModelVertices", vertices.data(),
		                                      vertices.size() * sizeof(OEMaths::vec3f),
		                                      VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
	Global::eventManager()->instantNotification<VulkanAPI::BufferUpdateEvent>(vertexEvent);

	// index data
	VulkanAPI::BufferUpdateEvent indexEvent{ "CubeModelIndices", indices.data(), indices.size() * sizeof(uint32_t),
		                                     VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
	Global::eventManager()->instantNotification<VulkanAPI::BufferUpdateEvent>(indexEvent);
}

void RenderableSkybox::createSkyboxPipeline(std::unique_ptr<VulkanAPI::Interface>& vkInterface,
                                            std::unique_ptr<RendererBase>& renderer,
                                            std::unique_ptr<ProgramState>& state, StateId::StateFlags& flags)
{
	// load shaders
	if (!state->shader.add(vkInterface->getDevice(), "env/Skybox/skybox-vert.spv", VulkanAPI::StageType::Vertex,
	                       "env/Skybox/skybox-frag.spv", VulkanAPI::StageType::Fragment))
	{
		LOGGER_ERROR("Unable to create skybox shaders.");
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
		if (layout.name == "CameraUbo")
		{
			vkInterface->getBufferManager()->enqueueDescrUpdate("Camera", &state->descriptorSet, layout.set,
			                                                    layout.binding, layout.type);
		}
	}

	// sort out the descriptor sets - images
	for (auto& layout : state->imageLayout.layouts)
	{
		// the shader must use these identifying names for uniform buffers -
		if (layout.name == "SkyboxSampler")
		{
			vkInterface->gettextureManager()->enqueueDescrUpdate("Skybox", &state->descriptorSet, &layout.sampler,
			                                                     layout.set, layout.binding);
		}
	}

	state->shader.pipelineLayoutReflect(state->pipelineLayout);
	state->pipelineLayout.create(vkInterface->getDevice(), state->descriptorLayout.getLayout());

	// create the graphics pipeline
	state->shader.pipelineReflection(state->pipeline);

	// only draw the skybox where there is no geometry
	state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eNotEqual, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
	                                            vk::StencilOp::eReplace, 0xff, 0x00, 1);

	state->pipeline.setDepthState(VK_FALSE, VK_FALSE, vk::CompareOp::eLessOrEqual);
	state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eNone);
	state->pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
	state->pipeline.setTopology(flags.topology);
	state->pipeline.addColourAttachment(VK_FALSE, renderer->getForwardPass());
	state->pipeline.create(vkInterface->getDevice(), renderer->getForwardPass(), state->shader, state->pipelineLayout,
	                       VulkanAPI::PipelineType::Graphics);
}

void RenderableSkybox::createSkyboxPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image,
                                        VulkanAPI::Texture& depthImage, vk::Device& device, vk::PhysicalDevice& gpu,
                                        RenderConfig& renderConfig)
{
	vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);

	// create the renderpasses and frame buffers
	renderpass.init(device);
	renderpass.addAttachment(renderConfig.deferred.deferredFormat, VulkanAPI::FinalLayoutType::Auto, false);
	renderpass.addAttachment(depthFormat, VulkanAPI::FinalLayoutType::Auto, false);
	renderpass.prepareRenderPass();

	// frame buffer prep
	std::vector<vk::ImageView> imageViews{ image.getImageView(), depthImage.getImageView() };
	renderpass.prepareFramebuffer(static_cast<uint32_t>(imageViews.size()), imageViews.data(),
	                              renderConfig.deferred.deferredWidth, renderConfig.deferred.deferredHeight, 1);
}

void RenderableSkybox::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, void* instance)
{
	SkyboxInstance* instanceData = (SkyboxInstance*)instance;

	ProgramState* state = instanceData->state;

	cmdBuffer.setViewport();
	cmdBuffer.setScissor();
	cmdBuffer.bindPipeline(state->pipeline);
	cmdBuffer.bindDescriptors(state->pipelineLayout, state->descriptorSet, VulkanAPI::PipelineType::Graphics);
	cmdBuffer.bindPushBlock(state->pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(float),
	                        &instanceData->blurFactor);

	vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
	cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
	cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer, instanceData->indexBuffer.offset);
	cmdBuffer.drawIndexed(instanceData->indexCount);
}
}    // namespace OmegaEngine
