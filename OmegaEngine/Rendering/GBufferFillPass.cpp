#include "GBufferFillPass.h"

#include "Components/RenderableManager.h"

#include "Models/MaterialInstance.h"

#include "Rendering/RenderQueue.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/common.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/Descriptors.h"

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
	vk::Format depthFormat = VulkanAPI::Util::getSupportedDepthFormat(context);

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

	builder.addExecute([=](RGraphContext& context) {
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
    Material* mat = render->material;
    assert(mat);
    
    // update the material descriptor set here as we have all the info we need.
    // This should be done only when completely nescessary as could impact performance
    if (!mat->descrSet)
    {
        VulkanAPI::DescriptorLayout* layout = prog->getDescrLayout();
        auto set = std::make_unique<VulkanAPI::DescriptorSet>();
        set->prepare(layout);
        // we need info from two places - image data which is the imageview and shader; and the bindings from
        // the shader program.
        // first grab the image data - try for all pbr materials, though if returns false, this isn't considered an error
        for (size_t i = 0; i < TextureType::Count; ++i)
        {
            Util::String id = mat->name.append(TextureGroup::texTypeToStr(i));
            VulkanAPI::Texture* tex = driver.getTexture2D(id);
        }
        
    }
    
	// merge the material set with the mesh ubo sets
	std::vector<vk::DescriptorSet> sets { prog->getDescrSet, render->material->descrSet };

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
