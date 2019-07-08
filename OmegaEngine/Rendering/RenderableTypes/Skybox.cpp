#include "Skybox.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkTextureManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/DataTypes/Texture.h"
#include "Rendering/Renderers/RendererBase.h"
#include "Rendering/RenderInterface.h"
#include "Engine/Omega_Global.h"
#include "Managers/EventManager.h"
#include "VulkanAPI/BufferManager.h"
#include "ObjectInterface/ComponentTypes.h"
#include "Rendering/RenderCommon.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	RenderableSkybox::RenderableSkybox(RenderInterface* renderInterface, SkyboxComponent& component, std::unique_ptr<VulkanAPI::BufferManager>& bufferManager) :
		RenderableBase(RenderTypes::Skybox)
	{
	    // upload the indices and verices to the gpu. This will happen per skybox
	    generateBuffers();

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
		skyboxInstance->indexCount = Models::Cube::indicesSize;
	}

	RenderableSkybox::~RenderableSkybox()
	{
	}

	void RenderableSkybox::generateBuffers()
	{
	    static constexpr uint32_t indicesSize = 36;
	    static constexpr uint32_t verticesSize = 24;

	    // cube vertices
	    static std::array<OEMaths::vec3f, verticesSize> vertices{
		    
		    OEMaths::vec3f{ -1.0f, -1.0f, 1.0f },
		    OEMaths::vec3f{ 1.0f, -1.0f, 1.0f },
		    OEMaths::vec3f{ 1.0f, 1.0f, 1.0f },
		    OEMaths::vec3f{ -1.0f, 1.0f, 1.0f },
		    OEMaths::vec3f{ -1.0f, -1.0f, -1.0f },
		    OEMaths::vec3f{ 1.0f, -1.0f, -1.0f },
		    OEMaths::vec3f{ 1.0f, 1.0f, -1.0f },   
			OEMaths::vec3f{ -1.0f, 1.0f, -1.0f }
	    };

	    // cube indices
	    static std::array<uint32_t, indicesSize> indices{ 
												   // front
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
	    VulkanAPI::BufferUpdateEvent indexEvent{ "CubeModelIndices", indices.data(),
		                                         indices.size() * sizeof(uint32_t),
		                                         VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
	    Global::eventManager()->instantNotification<VulkanAPI::BufferUpdateEvent>(indexEvent);
	}

	void RenderableSkybox::createSkyboxPipeline(vk::Device& device,
		std::unique_ptr<RendererBase>& renderer,
		std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
		std::unique_ptr<VulkanAPI::VkTextureManager>& textureManager,
		std::unique_ptr<ProgramState>& state)
	{
		// load shaders
		if (!state->shader.add(device, "env/Skybox/skybox-vert.spv", VulkanAPI::StageType::Vertex, "env/Skybox/skybox-frag.spv", VulkanAPI::StageType::Fragment)) 
		{
			LOGGER_ERROR("Unable to create skybox shaders.");
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
			if (layout.name == "CameraUbo")
			{
				bufferManager->enqueueDescrUpdate("Camera", &state->descriptorSet, layout.set, layout.binding, layout.type);
			}
		}

		// sort out the descriptor sets - images
		for (auto& layout : state->imageLayout.layouts) 
		{
			// the shader must use these identifying names for uniform buffers -
			if (layout.name == "SkyboxSampler") 
			{
				textureManager->enqueueDescrUpdate("Skybox", &state->descriptorSet, &layout.sampler, layout.set, layout.binding);
			}
		}

		state->shader.pipelineLayoutReflect(state->pipelineLayout);
		state->pipelineLayout.create(device, state->descriptorLayout.getLayout());

		// create the graphics pipeline
		state->shader.pipelineReflection(state->pipeline);

		// only draw the skybox where there is no geometry
		state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eNotEqual, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eReplace, 0xff, 0x00, 1);;

		state->pipeline.setDepthState(VK_FALSE, VK_FALSE, vk::CompareOp::eLessOrEqual);
		state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eBack);
		state->pipeline.setRasterFrontFace(vk::FrontFace::eCounterClockwise);
		state->pipeline.setTopology(vk::PrimitiveTopology::eTriangleList);
		state->pipeline.addColourAttachment(VK_FALSE, renderer->getForwardPass());
		state->pipeline.create(device, renderer->getForwardPass(), state->shader, state->pipelineLayout, VulkanAPI::PipelineType::Graphics);
	}

	void RenderableSkybox::createSkyboxPass(VulkanAPI::RenderPass& renderpass, VulkanAPI::Texture& image, VulkanAPI::Texture& depthImage, 
		vk::Device& device, vk::PhysicalDevice& gpu, RenderConfig& renderConfig)
	{
		vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);

		// create the renderpasses and frame buffers
		renderpass.init(device);
		renderpass.addAttachment(vk::ImageLayout::eShaderReadOnlyOptimal, renderConfig.deferred.deferredFormat, false);
		renderpass.addAttachment(vk::ImageLayout::eDepthStencilAttachmentOptimal, depthFormat, false);
		renderpass.prepareRenderPass();

		// frame buffer prep
		std::vector<vk::ImageView> imageViews{ image.getImageView() , depthImage.getImageView() };
		renderpass.prepareFramebuffer(static_cast<uint32_t>(imageViews.size()), imageViews.data(), renderConfig.deferred.deferredWidth, renderConfig.deferred.deferredHeight, 1);
	}

	void RenderableSkybox::render(VulkanAPI::SecondaryCommandBuffer& cmdBuffer, 
								void* instance)
	{
		SkyboxInstance* instanceData = (SkyboxInstance*)instance;

		ProgramState* state = instanceData->state;

		cmdBuffer.setViewport();
		cmdBuffer.setScissor();
		cmdBuffer.bindPipeline(state->pipeline);
		cmdBuffer.bindDescriptors(state->pipelineLayout, state->descriptorSet, VulkanAPI::PipelineType::Graphics);
		cmdBuffer.bindPushBlock(state->pipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(float), &instanceData->blurFactor);

		vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
		cmdBuffer.bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
		cmdBuffer.bindIndexBuffer(instanceData->indexBuffer.buffer, instanceData->indexBuffer.offset);
		cmdBuffer.drawIndexed(instanceData->indexCount);
	}
}
