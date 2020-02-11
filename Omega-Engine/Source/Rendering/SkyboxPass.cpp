#include "SkyboxPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "Types/Skybox.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"
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

bool SkyboxPass::prepare(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "skybox.glsl";
    VulkanAPI::ProgramManager::ShaderHash key {filename.c_str(), 0};
    VulkanAPI::ShaderProgram* prog = manager->getVariant(key);

    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    offscreenTex = builder.createTexture(2048, 2048, vk::Format::eR8G8B8A8Unorm);
    builder.addOutputAttachment("SkyboxPass", offscreenTex);

    // everything required to draw the skybox to the cmd buffer
    builder.addExecute([&](RGraphContext& context) {

        VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
        context.cmdBuffer->bindPipeline(renderpass, prog);

        context.cmdBuffer->bindDescriptors(prog, VulkanAPI::Pipeline::Type::Graphics);
        context.cmdBuffer->bindPushBlock(
            prog, vk::ShaderStageFlagBits::eFragment, sizeof(float), &skybox.blurFactor);

        context.cmdBuffer->bindVertexBuffer(skybox.vertexBuffer->get(), 0);
        context.cmdBuffer->bindIndexBuffer(skybox.indexBuffer->get(), 0);
        context.cmdBuffer->drawIndexed(OESkybox::indicesSize);
    });

    return true;
}

} // namespace OmegaEngine
