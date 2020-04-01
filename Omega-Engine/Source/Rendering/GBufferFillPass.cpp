#include "GBufferFillPass.h"

#include "Components/RenderableManager.h"
#include "Core/engine.h"
#include "ModelImporter/MaterialInstance.h"
#include "ModelImporter/MeshInstance.h"
#include "Rendering/RenderQueue.h"
#include "RenderGraph/RenderGraphPass.h"
#include "RenderGraph/RenderGraph.h"
#include "Scripting/OEConfig.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/Logger.h"
#include "utility/Compiler.h"

namespace OmegaEngine
{

GBufferFillPass::GBufferFillPass(
    VulkanAPI::VkDriver& driver,
    RenderGraph& rGraph,
    Util::String id,
    OERenderableManager& rendManager,
    EngineConfig& config)
    : RenderStageBase(id)
    , driver(driver)
    , vkContext(driver.getContext())
    , rGraph(rGraph)
    , rendManager(rendManager)
    , config(config)
{
}

bool GBufferFillPass::prepare(VulkanAPI::ProgramManager* manager)
{
    OE_UNUSED(manager);
    
    // shaders are prepared within the renderable manager for this pass
    // a list of the formats required for each buffer
    vk::Format depthFormat = VulkanAPI::VkUtil::getSupportedDepthFormat(vkContext.physical);

    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    // create the gbuffer textures
    gbufferInfo.tex.position = builder.createRenderTarget("pos_target", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.colour = builder.createRenderTarget("colour_target", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.normal = builder.createRenderTarget("normal_target", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.pbr = builder.createRenderTarget("pbr_target", 2048, 2048, vk::Format::eR16G16Sfloat);
    gbufferInfo.tex.emissive =
        builder.createRenderTarget("emissive_target", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.depth = builder.createRenderTarget("depth_target", 2048, 2048, depthFormat);

    // create the output taragets
    gbufferInfo.attach.position = builder.addWriter("position", gbufferInfo.tex.position);
    gbufferInfo.attach.colour = builder.addWriter("colour", gbufferInfo.tex.colour);
    gbufferInfo.attach.normal = builder.addWriter("normal", gbufferInfo.tex.normal);
    gbufferInfo.attach.pbr = builder.addWriter("pbr", gbufferInfo.tex.pbr);
    gbufferInfo.attach.emissive = builder.addWriter("emissive", gbufferInfo.tex.emissive);
    gbufferInfo.attach.depth = builder.addWriter("depth", gbufferInfo.tex.depth);

    OEMaths::colour4 clear = config.findOrInsertVec4("clearValue", OEEngine::Default_ClearVal);
    builder.setClearColour(clear);
    builder.setDepthClear(1.0f);

    builder.addExecute([=](RGraphContext& context) {
        // draw the contents of the renderable rendder queue
        OERenderer* renderer = context.renderer;
        renderer->drawQueueThreaded(driver.getCbManager(), context);
    });

    return true;
}

void GBufferFillPass::drawCallback(VulkanAPI::CmdBuffer* cmdBuffer, void* data, RGraphContext& context)
{
    Renderable* render = static_cast<Renderable*>(data);
    VulkanAPI::ShaderProgram* prog = render->program;
    Material* mat = render->material;

    assert(render && mat && prog);

    auto& cbManager = context.driver->getCbManager();

    VulkanAPI::ShaderBinding::SamplerBinding matBinding =
        prog->findSamplerBinding(mat->instance->name, VulkanAPI::Shader::Type::Fragment);

    VulkanAPI::DescriptorSetInfo* matSetInfo = cbManager.findDescriptorSet(mat->materialHash, matBinding.set);
    assert(matSetInfo);

    // merge the material set with the mesh ubo sets
    uint8_t setCount = prog->getSetCount();
    assert(setCount > 0);
    std::vector<vk::DescriptorSet> uboSets(setCount);

    std::vector<VulkanAPI::DescriptorSetInfo> uboSetInfo = cbManager.findDescriptorSets(prog->getShaderId());
    assert(!uboSetInfo.empty());
    
    std::vector<vk::DescriptorSet> descrSets(uboSetInfo.size() + 1);
    for (uint64_t i = 0; i < uboSetInfo.size(); ++i)
    {
        descrSets[i] = uboSetInfo[i].descrSet;
    }
    descrSets[descrSets.size() - 1] = matSetInfo->descrSet;

    // ==================== bindings ==========================

    VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
    cmdBuffer->bindPipeline(cbManager, renderpass, prog);

    cmdBuffer->bindDynamicDescriptors(cbManager,
        prog, render->dynamicOffset, VulkanAPI::Pipeline::Type::Graphics);

    // the push block contains all the material attributes for this mesh
    cmdBuffer->bindPushBlock(
        prog,
        vk::ShaderStageFlagBits::eFragment,
        sizeof(MaterialInstance::MaterialBlock),
        &render->material->instance->block);

    cmdBuffer->bindVertexBuffer(render->vertBuffer->get(), 0);
    cmdBuffer->bindIndexBuffer(render->indexBuffer->get(), 0);

    // draw all the primitives
    for (const MeshInstance::Primitive& prim : render->instance->primitives)
    {
        cmdBuffer->drawIndexed(prim.indexPrimitiveCount, prim.indexPrimitiveOffset);
    }
}


} // namespace OmegaEngine
