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

#include "Renderer.h"

#include "Components/RenderableManager.h"
#include "Core/Scene.h"
#include "Core/engine.h"
#include "RenderGraph/RenderGraph.h"
#include "Rendering/CompositionPass.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/IndirectLighting.h"
#include "Rendering/LightingPass.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/SkyboxPass.h"
#include "Scripting/OEConfig.h"
#include "Threading/ThreadPool.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

OERenderer::OERenderer(
    OEEngine& eng, OEScene& scene, VulkanAPI::Swapchain& swapchain, EngineConfig& config)
    : vkDriver(eng.getVkDriver())
    , rGraph(std::make_unique<RenderGraph>(&vkDriver, this))
    , swapchain(swapchain)
    , engine(eng)
    , scene(scene)
    , config(config)
{
}

OERenderer::~OERenderer()
{
}

bool OERenderer::prepare()
{
    // TODO: At the moment only a deffered renderer is supported. Maybe add a forward renderer as
    // well?!
    for (const RenderStage& stage : deferredStages)
    {
        switch (stage)
        {
            case RenderStage::GBufferFill:
                rStages.emplace_back(std::make_unique<GBufferFillPass>(
                    vkDriver, *rGraph, "Stage_GB", *engine.getRendManager(), config));
                break;
            case RenderStage::LightingPass:
                rStages.emplace_back(std::make_unique<LightingPass>(vkDriver.getContext(), *rGraph, "Stage_Light"));
                break;
            case RenderStage::Skybox:
                rStages.emplace_back(std::make_unique<SkyboxPass>(*rGraph, "Stage_PostGB", scene));
                break;
            case RenderStage::Composition:
                rStages.emplace_back(
                    std::make_unique<CompositionPass>(vkDriver, *rGraph, "Stage_Comp", swapchain));
                break;
        }
    }

    // initialise all the stages
    for (const auto& stage : rStages)
    {
        if (!stage->init(&engine.getVkDriver().getProgManager()))
        {
            return false;
        }
    }

    return true;
}

void OERenderer::preparePasses()
{
    for (auto& stage : rStages)
    {
        stage->setupPass();
    }
}

void OERenderer::beginFrame()
{
    // begin the frame on the driver side
    vkDriver.beginFrame(swapchain);

    // clear the render graph
    rGraph->reset();

    // set up the render graph for this frame
    preparePasses();
}

bool OERenderer::draw()
{
    beginFrame();

    // optimisation and compilation of the render graph. If nothing has changed since the last frame
    // then this call will just return.
    if (!rGraph->compile())
    {
        return false;
    }

    // check if indirect lighting component needs init/updating
    if (scene.ibl && scene.ibl->needsUpdating())
    {
        if (!scene.ibl->prepare())
        {
            return false;
        }
    }

    // at this point we have all the images/buffers prepared so udate the descriptors now. This
    // happens on the first frame and if the images/buffer handles change
    if (!vkDriver.getCbManager().updateAllShaderDecsriptorSets())
    {
        return false;
    }

    // executes the user-defined callback for all of the passes in-turn.
    rGraph->execute();

    // finally send to the swap-chain for presentation
    vkDriver.endFrame(swapchain);

    return true;
}

void OERenderer::drawQueueThreaded(
    VulkanAPI::CBufferManager& manager,
    RGraphContext& rgraphContext,
    RGraphPassContext& rpassContext)
{
    auto queue = scene.renderQueue.getQueue(RenderQueue::Type::Colour);

    VulkanAPI::CmdBuffer* cmdBuffer = manager.getCmdBuffer();
    rgraphContext.driver->beginRenderpass(cmdBuffer, *rpassContext.rpass, *rpassContext.fbo, true, true);

    auto thread_draw = [&queue, &rgraphContext, &rpassContext, &manager](size_t start, size_t end) {
        assert(end <= queue.size());
        assert(start < end);
        for (size_t idx = start; idx < end; ++idx)
        {
            RenderableQueueInfo& info = queue[idx];

            // a cmd pool per thread with a buffer
            VulkanAPI::CmdBuffer* cbSecondary = manager.getSecondaryCmdBuffer();
            cbSecondary->beginSecondary(*rpassContext.rpass, *rpassContext.fbo);

            // use custom defined viewing area - at the moment set to the framebuffer size
            vk::Viewport viewport {0.0f,
                                   0.0f,
                                   static_cast<float>(rpassContext.fbo->getWidth()),
                                   static_cast<float>(rpassContext.fbo->getHeight()),
                                   0.0f,
                                   1.0f};
            cbSecondary->setViewport(viewport);

            vk::Rect2D scissor {
                {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)},
                {static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height)}};
            cbSecondary->setScissor(scissor);

            info.renderFunction(cbSecondary, info.renderableData, rgraphContext, rpassContext);
            cbSecondary->end();
        }
    };

    // split task up equally per thtread - using a new secondary cmd buffer per thread
    size_t workSize = queue.size();
    ThreadTaskSplitter split {0, workSize, thread_draw};
    split.run();

    // check all task have finished here before execute?
    manager.executeSecondaryCommands();

    rgraphContext.driver->endRenderpass(cmdBuffer);
}

// ==================== front-end =============================

bool Renderer::prepare()
{
    return static_cast<OERenderer*>(this)->prepare();
}

void Renderer::beginFrame()
{
    static_cast<OERenderer*>(this)->beginFrame();
}

void Renderer::draw()
{
    static_cast<OERenderer*>(this)->draw();
}

} // namespace OmegaEngine
