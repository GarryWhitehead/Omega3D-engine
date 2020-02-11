#include "GBufferFillPass.h"

#include "Components/RenderableManager.h"
#include "Core/Engine.h"
#include "ModelImporter/MaterialInstance.h"
#include "ModelImporter/MeshInstance.h"
#include "Rendering/RenderQueue.h"
#include "RenderGraph/RenderGraphPass.h"
#include "RenderGraph/RenderGraph.h"
#include "Scripting/OEConfig.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/Logger.h"

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
    // shaders are prepared within the renderable manager for this pass

    // a list of the formats required for each buffer
    vk::Format depthFormat = VulkanAPI::VkUtil::getSupportedDepthFormat(vkContext.getGpu());

    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

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

    OEMaths::colour4 clear = config.findOrInsertVec4("clearValue", OEEngine::Default_ClearVal);
    builder.setClearColour(clear);
    builder.setDepthClear(1.0f);

    builder.addExecute([=](RGraphContext& context) {
        // draw the contents of the renderable rendder queue
        OERenderer* renderer = context.renderer;
        renderer->drawQueueThreaded(*context.cmdBuffer, driver.getCbManager(), context);
    });

    return true;
}

void GBufferFillPass::drawCallback(
    VulkanAPI::CmdBuffer& cmdBuffer, void* data, RGraphContext& context)
{
    Renderable* render = static_cast<Renderable*>(data);
    VulkanAPI::ShaderProgram* prog = render->program;
    Material* mat = render->material;

    assert(render && mat && prog);

    // update the material descriptor set here, if required, as we have all the info we need.
    // This should be done only when completely nescessary as will impact performance
    if (!mat->descrSet)
    {
        // get the bindings for the material
        VulkanAPI::ShaderBinding::SamplerBinding matBinding =
            prog->findSamplerBinding(mat->instance->name, VulkanAPI::Shader::Type::Fragment);

        // get the layout and create the descriptor set
        VulkanAPI::DescriptorLayout* layout = prog->getDescrLayout(matBinding.set);
        mat->descrSet->addLayout(*layout);

        // now update the material set with all the textures that have been defined
        // Note: if null, this isn't a error as not all pbr materials have the full range of types
        for (uint32_t i = 0; i < MaterialInstance::TextureType::Count; ++i)
        {
            Util::String id = Util::String::append(
                mat->instance->name,
                TextureGroup::texTypeToStr(static_cast<MaterialInstance::TextureType>(i)));
            VulkanAPI::Texture* tex = context.driver->getTexture2D(id);
            if (tex)
            {
                mat->descrSet->updateImageSet(
                    matBinding.set,
                    i,
                    matBinding.type,
                    tex->getSampler()->get(),
                    tex->getImageView()->get(),
                    tex->getImageLayout());
            }
        }
    }

    // merge the material set with the mesh ubo sets
    auto descrSets = prog->getDescrSet()->getOrdered();
    auto matSet = mat->descrSet->getOrdered();
    std::copy(matSet.begin(), matSet.end(), descrSets.end());

    // ==================== bindings ==========================

    VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
    cmdBuffer.bindPipeline(renderpass, prog);

    cmdBuffer.bindDynamicDescriptors(
        prog, render->dynamicOffset, VulkanAPI::Pipeline::Type::Graphics);

    // the push block contains all the material attributes for this mesh
    cmdBuffer.bindPushBlock(
        prog,
        vk::ShaderStageFlagBits::eFragment,
        sizeof(MaterialInstance::MaterialBlock),
        &render->material->instance->block);

    cmdBuffer.bindVertexBuffer(render->vertBuffer->get(), 0);
    cmdBuffer.bindIndexBuffer(render->indexBuffer->get(), 0);

    // draw all the primitives
    for (const MeshInstance::Primitive& prim : render->instance->primitives)
    {
        cmdBuffer.drawIndexed(prim.indexPrimitiveCount, prim.indexPrimitiveOffset);
    }
}


} // namespace OmegaEngine
