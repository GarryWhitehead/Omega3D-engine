/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "SkyboxPass.h"

#include "Core/Scene.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "Types/Skybox.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

SkyboxPass::SkyboxPass(RenderGraph& rGraph, Util::String id, OEScene& scene)
    : RenderStageBase(id), rGraph(rGraph), scene(scene)

{
}

SkyboxPass::~SkyboxPass()
{
}

bool SkyboxPass::init(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "skybox.glsl";
    prog = manager->getVariantOrCreate(filename);
    if (!prog)
    {
        return false;
    }
    return true;
}

void SkyboxPass::setupPass()
{
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    offscreenTex =
        builder.createRenderTarget("compositionRT", 2048, 2048, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, VulkanAPI::LoadClearFlags::DontCare, VulkanAPI::LoadClearFlags::DontCare);
    builder.addReader("lighting");
    builder.addWriter("skybox", offscreenTex);

    // everything required to draw the skybox to the cmd buffer
    builder.addExecute([=](RGraphPassContext& rpassContext, RGraphContext& rgraphContext) {
        auto& cbManager = rgraphContext.driver->getCbManager();
        VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getCmdBuffer();

        rgraphContext.driver->beginRenderpass(cmdBuffer, *rpassContext.rpass, *rpassContext.fbo, false);

        cmdBuffer->bindPipeline(
            cbManager,
            rpassContext.rpass,
            rpassContext.fbo,
            prog,
            VulkanAPI::Pipeline::Type::Graphics);

        cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);
        cmdBuffer->bindPushBlock(
            prog,
            vk::ShaderStageFlagBits::eFragment,
            sizeof(float),
            &scene.getSkybox()->blurFactor);

        cmdBuffer->bindVertexBuffer(scene.getSkybox()->vertexBuffer->get(), 0);
        cmdBuffer->bindIndexBuffer(scene.getSkybox()->indexBuffer->get(), 0);
        cmdBuffer->drawIndexed(OESkybox::indicesSize);

        rgraphContext.driver->endRenderpass(cmdBuffer);
    });
}

} // namespace OmegaEngine
