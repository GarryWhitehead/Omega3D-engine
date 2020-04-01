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
    , rGraph(std::make_unique<RenderGraph>(vkDriver))
    , swapchain(swapchain)
    , engine(eng)
    , scene(scene)
    , config(config)
{
}

OERenderer::~OERenderer()
{
}

void OERenderer::prepare()
{
    // TODO: At the moment only a deffered renderer is supported. Maybe add a forward renderer as
    // well?!
    for (const RenderStage& stage : deferredStages)
    {
        switch (stage)
        {
            case RenderStage::IndirectLighting:
                rStages.emplace_back(
                    std::make_unique<IndirectLighting>(*rGraph, "Stage_IL", *scene.skybox));
                break;
            case RenderStage::GBufferFill:
                rStages.emplace_back(std::make_unique<GBufferFillPass>(
                    vkDriver, *rGraph, "Stage_GB", *engine.getRendManager(), config));
                break;
            case RenderStage::LightingPass:
                rStages.emplace_back(std::make_unique<LightingPass>(*rGraph, "Stage_Light"));
                break;
            case RenderStage::Skybox:
                rStages.emplace_back(
                    std::make_unique<SkyboxPass>(*rGraph, "Stage_PostGB", *scene.skybox));
                break;
        }
    }

    // the last stage is always the composition pass - writes to the surface
    rStages.emplace_back(std::make_unique<CompositionPass>(vkDriver, *rGraph, "Stage_Comp", swapchain));
}

bool OERenderer::preparePasses()
{
    for (auto& stage : rStages)
    {
        if (!stage->prepare(&engine.getVkDriver().getProgManager()))
        {
            return false;
        }
    }
    
    return true;
}

void OERenderer::beginFrame()
{
}

bool OERenderer::update()
{
    // optimisation and compilation of the render graph. If nothing has changed since the last frame
    // then this call will just return.
    if (!rGraph->prepare())
    {
        return false;
    }
    return true;
}

void OERenderer::draw()
{
    vkDriver.beginFrame(swapchain);
    
    preparePasses();

    // executes the user-defined callback for all of the passes in-turn.
    rGraph->execute();

    // finally send to the swap-chain presentation
    vkDriver.endFrame(swapchain);
}

void OERenderer::drawQueueThreaded(VulkanAPI::CBufferManager& manager, RGraphContext& context)
{
    VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
    VulkanAPI::FrameBuffer* fbuffer = context.rGraph->getFramebuffer(context.framebuffer);
    
    auto queue = scene.renderQueue.getQueue(RenderQueue::Type::Colour);

    auto thread_draw = [&queue, &context, &renderpass, &fbuffer, &manager](size_t start, size_t end) {
        assert(end < queue.size());
        assert(start < end);
        for (size_t idx = start; idx < end; ++idx)
        {
            RenderableQueueInfo& info = queue[idx];
            
             // a cmd pool per thread with a buffer
            VulkanAPI::CmdBuffer* cbSecondary = manager.createSecondaryCmdBuffer();
            cbSecondary->beginSecondary(*renderpass, *fbuffer);
            
            info.renderFunction(cbSecondary, info.renderableData, context);
        }
    };

    // split task up equally per thtread - using a new secondary cmd buffer per thread
    size_t workSize = queue.size();
    ThreadTaskSplitter split {0, workSize, thread_draw};
    split.run();

    // check all task have finished here before execute?
    manager.executeSecondaryCommands();
}

// ==================== front-end =============================

void Renderer::prepare()
{
    static_cast<OERenderer*>(this)->prepare();
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
