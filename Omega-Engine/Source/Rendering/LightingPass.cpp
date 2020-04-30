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

#include "LightingPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Shader.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

LightingPass::LightingPass(RenderGraph& rGraph, Util::String id)
    : RenderStageBase(id.c_str()), rGraph(rGraph)
{
}

bool LightingPass::init(VulkanAPI::ProgramManager* manager)
{
    // load the shaders
    const Util::String filename = "lighting.glsl";
    prog = manager->getVariantOrCreate(filename, 0);
    if (!prog)
    {
        return false;
    }
    // temp measure for now - needs adding to configs
    prog->addVariant("IBL_ENABLED", 1, VulkanAPI::Shader::Type::Fragment);
    return true;
}

void LightingPass::setupPass()
{
    // build the lighting render pass
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    // read from the gbuffer targets
    builder.addReader("position");
    builder.addReader("colour");
    builder.addReader("normal");
    builder.addReader("pbr");
    builder.addReader("emissive");

    // create the gbuffer textures
    passInfo.output =
        builder.createRenderTarget("lightingRT", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);

    // create the output taragets
    passInfo.output = builder.addWriter("lighting", passInfo.output);

    builder.addExecute([=](RGraphPassContext& rpassContext, RGraphContext& rgraphContext) {
        auto& cbManager = rgraphContext.driver->getCbManager();
        VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getCmdBuffer();

        // bind the pipeline
        rgraphContext.driver->beginRenderpass(cmdBuffer, *rpassContext.rpass, *rpassContext.fbo);

        cmdBuffer->bindPipeline(
            cbManager,
            rpassContext.rpass,
            rpassContext.fbo,
            prog,
            VulkanAPI::Pipeline::Type::Graphics);

        // bind the descriptor
        cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);
        // cmdBuffer->bindPushBlock(prog, &renderConfig.ibl);

        // render full screen quad to screen
        cmdBuffer->drawQuad();

        rgraphContext.driver->endRenderpass(cmdBuffer);
    });
}

} // namespace OmegaEngine
