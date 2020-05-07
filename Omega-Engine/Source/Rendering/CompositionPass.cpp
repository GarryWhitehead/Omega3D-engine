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

#include "CompositionPass.h"

#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Utility.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

CompositionPass::CompositionPass(
    VulkanAPI::VkDriver& driver,
    RenderGraph& rGraph,
    Util::String id,
    VulkanAPI::Swapchain& swapchain)
    : RenderStageBase(id), driver(driver), rGraph(rGraph), swapchain(swapchain)
{
}

CompositionPass::~CompositionPass()
{
}

bool CompositionPass::init(VulkanAPI::ProgramManager* manager)
{
    depthFormat = VulkanAPI::VkUtil::getSupportedDepthFormat(driver.getContext().physical);

    // load the shaders
    const Util::String filename = "composition.glsl";
    prog = manager->getVariantOrCreate(filename);
    if (!prog)
    {
        return false;
    }
    return true;
}

void CompositionPass::setupPass()
{
    const Util::String passId = "compositionPass";
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    uint32_t currentImageIndex = driver.getCurrentImageIndex();

    // final pass, write to the surface
    // TODO: need to implement user defined sample size - set to one for now
    backBuffer = builder.importRenderTarget(
        "surface",
        swapchain.getExtentsWidth(),
        swapchain.getExtentsHeight(),
        swapchain.getFormat(),
        1,
        swapchain.getImageView(currentImageIndex));

    depth = builder.createRenderTarget(
        "composition_depthRT",
        swapchain.getExtentsWidth(),
        swapchain.getExtentsHeight(),
        depthFormat);

    builder.addReader("skybox");
    builder.addWriter("composition", backBuffer);
    builder.addWriter("composition_depth", depth);

    builder.addExecute([=](RGraphPassContext& rpassContext, RGraphContext& rgraphContext) {
        auto& cbManager = rgraphContext.driver->getCbManager();
        VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getScCommandBuffer(currentImageIndex);

        // bind the pipeline
        rgraphContext.driver->beginRenderpass(cmdBuffer, *rpassContext.rpass, *rpassContext.fbo);

        cmdBuffer->bindPipeline(
            cbManager,
            rpassContext.rpass,
            rpassContext.fbo,
            prog,
            VulkanAPI::Pipeline::Type::Graphics);
        cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);

        // TODO: this needs to be user defined
        struct PushBlock
        {
            float expBias = 1.0f;
            float gamma = 2.2f;
        } pushBlock;

        cmdBuffer->bindPushBlock(
            prog, vk::ShaderStageFlagBits::eFragment, sizeof(PushBlock), &pushBlock);

        // render full screen quad to screen
        cmdBuffer->drawQuad();

        rgraphContext.driver->endRenderpass(cmdBuffer);
    });
}

} // namespace OmegaEngine
