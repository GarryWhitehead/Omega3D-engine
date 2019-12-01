#include "GBufferFillPass.h"

#include "RenderGraph/RenderGraph.h"

#include "Components/RenderableManager.h"

#include "Models/MaterialInstance.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/common.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

GBufferFillPass::GBufferFillPass(RenderGraph& rGraph, Util::String id, RenderableManager& rendManager)
    : rGraph(rGraph)
    , rendManager(rendManager)
    , RenderStageBase(id)
{
}

bool GBufferFillPass::prepare(VulkanAPI::ProgramManager* manager)
{
	// shaders are prepared within the renderable manager for this pass

	// a list of the formats required for each buffer
	vk::Format depthFormat = VulkanAPI::VkDriver::getDepthFormat(gpu);

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

	builder.setClearColour();
	builder.setDepthClear();

	builder.addExecute([](RGraphContext& context) {
		// for me old sanity!
		assert(context.cbManager);
		assert(context.renderer);
		auto& cmdBuffer = context.cbManager->getCmdBuffer(context.cmdBuffer);

		// draw the contents of the renderable rendder queue
		Renderer* renderer = context.renderer;
		renderer->drawQueueThreaded(*cmdBuffer, RenderQueue::Type::Colour);
	});
}

void GBufferFillPass::drawCallback(VulkanAPI::CmdBuffer& cmdBuffer, void* data, RGraphContext& context)
{
	Renderable* render = static_cast<Renderable*>(data);

	// merge the material set with the mesh ubo sets
	std::vector<vk::DescriptorSet> sets { state->descriptorSet.get(), render->material->descrSet };

	VulkanAPI::ProgramManager* pgManager = context.ProgramManager;
	VulkanAPI::CmdBufferManager* cbManager = context.cbManager;

	// ==================== bindings ==========================

	cmdBuffer.bindPipeline(context.rpass, prog);

	cmdBuffer.bindDynamicDescriptors(prog, dynamicOffsets, VulkanAPI::Pipeline::Type::Graphics);

	cmdBuffer.bindPushBlock(prog, vk::ShaderStageFlagBits::eFragment, sizeof(MaterialInstance::MaterialBlock),
	                        &render->material->instance.block);

	vk::DeviceSize offset = { render->vertexBuffer.offset };
	cmdBuffer.bindVertexBuffer(render->vertexBuffer->get(), offset);
	cmdBuffer.bindIndexBuffer(render->indexBuffer->get(),
	                          render->indexBuffer.offset + (render->indexOffset * sizeof(uint32_t)));

	// draw all primitives
	for (const MeshInstance::Primitive& prim : render->instance.primitives)
	{
		cmdBuffer.drawIndexed(prim.indexPrimitiveCount, prim.indexPrimitiveOffset);
	}
}


}    // namespace OmegaEngine
