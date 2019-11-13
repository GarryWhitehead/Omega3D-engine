#include "GBufferFillPass.h"

#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Managers/ShaderManager.h"
#include "VulkanAPI/common.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

GBufferFillPass::GBufferFillPass(RenderGraph& rGraph, Util::String id)
    : rGraph(rGraph)
    , RenderStageBase(id)
{
}

bool GBufferFillPass::prepare(VulkanAPI::ShaderManager* manager)
{
	// load the shaders
	// load the shaders
    VulkanAPI::ShaderProgram* program = manager->findOrCreateShader("renderer/deferred/lighting.glsl", nullptr, 0);
    if (!program)
    {
        LOGGER_ERROR("Fatal error whilst trying to compile shader for renderpass: %s.", passId.c_str());
        return false;
    }

	// a list of the formats required for each buffer
	vk::Format depthFormat = VulkanAPI::VkContext::getDepthFormat(gpu);

	RenderGraphBuilder builder = rGraph.createRenderPass(passId);

    // create the gbuffer textures
    gbufferInfo.tex.position = builder.createTexture(2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.colour = builder.createTexture(2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.normal = builder.createTexture(2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.pbr = builder.createTexture(2048, 2048, vk::Format::eR16G16Sfloat);
    gbufferInfo.tex.emissive = builder.createTexture(2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.depth = builder.createTexture(2048, 2048, depthFormat);

    // create the output taragets
    gbufferInfo.attach.position = builder.addOutputAttachment("position", gbufferInfo.tex.position);
    gbufferInfo.attach.colour = builder.addOutputAttachment("colour", gbufferInfo.tex.colour);
    gbufferInfo.attach.normal = builder.addOutputAttachment("normal", gbufferInfo.tex.normal);
    gbufferInfo.attach.emissive = builder.addOutputAttachment("emissive", gbufferInfo.tex.emissive);
    gbufferInfo.attach.pbr = builder.addOutputAttachment("pbr", gbufferInfo.tex.pbr);
    gbufferInfo.attach.depth = builder.addOutputAttachment("depth", gbufferInfo.tex.depth);


	builder.addExecute([](RenderInfo& rInfo, RGraphContext& context) {
		MeshInstance* instanceData = (MeshInstance*)instance;

		std::vector<uint32_t> dynamicOffsets{ instanceData->transformDynamicOffset };
		if (instanceData->type == StateMesh::Skinned)
		{
			dynamicOffsets.push_back(instanceData->skinnedDynamicOffset);
		}

		// merge the material set with the mesh ubo sets
		std::vector<vk::DescriptorSet> materialSet = instanceData->descriptorSet.get();
		std::vector<vk::DescriptorSet> meshSet = state->descriptorSet.get();
		meshSet.insert(meshSet.end(), materialSet.begin(), materialSet.end());

		context.cmdBuffer->setViewport();
		context.cmdBuffer->setScissor();
		context.cmdBuffer->bindPipeline(state->pipeline);

		context.cmdBuffer->bindDynamicDescriptors(state->pipelineLayout, meshSet, VulkanAPI::PipelineType::Graphics,
		                                          dynamicOffsets);
		context.cmdBuffer->bindPushBlock(state->pipelineLayout, vk::ShaderStageFlagBits::eFragment,
		                                 sizeof(MeshInstance::MaterialPushBlock), &instanceData->materialPushBlock);

		vk::DeviceSize offset = { instanceData->vertexBuffer.offset };
		context.cmdBuffer->bindVertexBuffer(instanceData->vertexBuffer.buffer, offset);
		context.cmdBuffer->bindIndexBuffer(instanceData->indexBuffer.buffer,
		                                   instanceData->indexBuffer.offset +
		                                       (instanceData->indexOffset * sizeof(uint32_t)));
		context.cmdBuffer->drawIndexed(instanceData->indexPrimitiveCount, instanceData->indexPrimitiveOffset);
	});
}


}    // namespace OmegaEngine
