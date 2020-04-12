#include "SkyboxPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "Types/Skybox.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Utility.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

SkyboxPass::SkyboxPass(RenderGraph& rGraph, Util::String id, OESkybox& skybox)
    : RenderStageBase(id), rGraph(rGraph), skybox(skybox)

{
}

SkyboxPass::~SkyboxPass()
{
}

bool SkyboxPass::init(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "skybox.glsl";
    prog = manager->getVariantOrCreate(filename, 0);
    if (!prog)
    {
        return false;
    }
    return true;
}

void SkyboxPass::setupPass()
{
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    offscreenTex = builder.createRenderTarget("skybox_target", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    builder.addReader("lighting");
    builder.addWriter("skybox", offscreenTex);

    // everything required to draw the skybox to the cmd buffer
    builder.addExecute([=](RGraphContext& context) {

        auto& cbManager = context.driver->getCbManager();
        VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getCmdBuffer();

        VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
        cmdBuffer->bindPipeline(cbManager, renderpass, prog);

        cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);
        cmdBuffer->bindPushBlock(
            prog, vk::ShaderStageFlagBits::eFragment, sizeof(float), &skybox.blurFactor);

        cmdBuffer->bindVertexBuffer(skybox.vertexBuffer->get(), 0);
        cmdBuffer->bindIndexBuffer(skybox.indexBuffer->get(), 0);
        cmdBuffer->drawIndexed(OESkybox::indicesSize);
    });
}

} // namespace OmegaEngine
