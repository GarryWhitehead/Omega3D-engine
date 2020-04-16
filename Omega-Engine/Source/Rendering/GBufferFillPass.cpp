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

bool GBufferFillPass::init(VulkanAPI::ProgramManager* manager)
{
    OE_UNUSED(manager);
    depthFormat = VulkanAPI::VkUtil::getSupportedDepthFormat(vkContext.physical);
    return true;
}

void GBufferFillPass::setupPass()
{
    // shaders are prepared within the renderable manager for this pass
    // a list of the formats required for each buffer
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    // create the gbuffer textures
    gbufferInfo.tex.position = builder.createRenderTarget("positionRT", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.colour = builder.createRenderTarget("baseColourRT", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.normal = builder.createRenderTarget("normalRT", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.pbr = builder.createRenderTarget("pbrRT", 2048, 2048, vk::Format::eR16G16Sfloat);
    gbufferInfo.tex.emissive =
        builder.createRenderTarget("emissiveRT", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.depth = builder.createRenderTarget("depthRT", 2048, 2048, depthFormat);

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

    builder.addExecute([=](RGraphPassContext& rpassContext, RGraphContext& rgraphContext) {
        // draw the contents of the renderable rendder queue
        OERenderer* renderer = rgraphContext.renderer;
        renderer->drawQueueThreaded(driver.getCbManager(), rgraphContext, rpassContext);
    });
}

void GBufferFillPass::drawCallback(VulkanAPI::CmdBuffer* cmdBuffer, void* data, RGraphContext& rgraphContext, RGraphPassContext& rpassContext)
{
    Renderable* render = static_cast<Renderable*>(data);
    VulkanAPI::ShaderProgram* prog = render->program;
    Material* mat = render->material;

    assert(render && mat && prog);

    auto& cbManager = rgraphContext.driver->getCbManager();

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

    VulkanAPI::RenderPass* renderpass = rgraphContext.rGraph->getRenderpass(rpassContext.rpass);
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
