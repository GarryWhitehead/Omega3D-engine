#include "Skybox.h"

#include "VulkanAPI/Managers/ProgramManager.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

Skybox::Skybox(RenderGraph& rGraph, Util::String id)
    : rGraph(rGraph)
    , RenderStageBase(id)
{
}

Skybox::~Skybox()
{
}

bool Skybox::prepare(VulkanAPI::ShaderManager* manager)
{
	vk::Format depthFormat = VulkanAPI::Device::getDepthFormat(gpu);

	// load the shaders
	VulkanAPI::ShaderProgram* program = manager->findOrCreateShader("skybox.glsl", nullptr, 0);
	if (!program)
	{
		LOGGER_ERROR("Fatal error whilst trying to compile shader for renderpass: %s.", passId);
		return false;
	}

	RenderGraphBuilder builder = rGraph.createRenderPass(passId);

	//  use the output from the lighting pass as a input
	builder.addInputAttachment("lighting");

	builder.addOutputAttachment("SkyboxPass", output);
}

/*void Skybox::generateBuffers()
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

		// only draw the skybox where there is no geometry
	state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eNotEqual, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
	                                            vk::StencilOp::eReplace, 0xff, 0x00, 1);

	state->pipeline.setDepthState(VK_FALSE, VK_FALSE, vk::CompareOp::eLessOrEqual);
	state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eNone);
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
}*/
}    // namespace OmegaEngine