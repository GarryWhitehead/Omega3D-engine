#include "Renderer.h"

#include "Components/RenderableManager.h"
#include "Core/engine.h"
#include "Core/Scene.h"
#include "RenderGraph/RenderGraph.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/IndirectLighting.h"
#include "Rendering/LightingPass.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/SkyboxPass.h"
#include "Rendering/CompositionPass.h"
#include "Scripting/OEConfig.h"
#include "Threading/ThreadPool.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CBufferManager.h"
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
                rStages.emplace_back(std::make_unique<LightingPass>(*rGraph, "Stage_Light"));
                break;
            case RenderStage::Skybox:
                rStages.emplace_back(
                    std::make_unique<SkyboxPass>(*rGraph, "Stage_PostGB", scene));
                break;
            case RenderStage::Composition:
                rStages.emplace_back(std::make_unique<CompositionPass>(vkDriver, *rGraph, "Stage_Comp", swapchain));
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
}

bool OERenderer::update()
{
    preparePasses();
    
    // optimisation and compilation of the render graph. If nothing has changed since the last frame
    // then this call will just return.
    if (!rGraph->prepare())
    {
        return false;
    }
    return true;
}

bool OERenderer::draw()
{
    vkDriver.beginFrame(swapchain);
    
    // check if indirect lighting component needs init/updating
    if (scene.ibl && scene.ibl->needsUpdating())
    {
        if (!scene.ibl->prepare())
        {
            return false;
        }
    }
    
    // at this point we have all the images/buffers prepared so udate the descriptors now. This happens on the first frame and if the
    // images/buffer handles change
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

void OERenderer::drawQueueThreaded(VulkanAPI::CBufferManager& manager, RGraphContext& rgraphContext, RGraphPassContext& rpassContext)
{
    RenderGraph* rGraph = rgraphContext.rGraph;
    VulkanAPI::RenderPass* renderpass = rGraph->getRenderpass(rpassContext.rpass);
    
    auto queue = scene.renderQueue.getQueue(RenderQueue::Type::Colour);

    VulkanAPI::CmdBuffer* cmdBuffer = manager.getCmdBuffer();
    rgraphContext.driver->beginRenderpass(cmdBuffer, *renderpass, true);
        
    auto thread_draw = [&queue, &rgraphContext, &rpassContext, &renderpass, &manager](size_t start, size_t end) {
        assert(end <= queue.size());
        assert(start < end);
        for (size_t idx = start; idx < end; ++idx)
        {
            RenderableQueueInfo& info = queue[idx];
            
             // a cmd pool per thread with a buffer
            VulkanAPI::CmdBuffer* cbSecondary = manager.getSecondaryCmdBuffer();
            cbSecondary->beginSecondary(*renderpass);
            
            // use custom defined viewing area - at the moment set to the framebuffer size
            vk::Viewport viewport {0.0f,
                                   0.0f,
                                   static_cast<float>(renderpass->getWidth()),
                                   static_cast<float>(renderpass->getHeight()),
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

void Renderer::update()
{
    static_cast<OERenderer*>(this)->update();
}

void Renderer::draw()
{
    static_cast<OERenderer*>(this)->draw();
}

} // namespace OmegaEngine
