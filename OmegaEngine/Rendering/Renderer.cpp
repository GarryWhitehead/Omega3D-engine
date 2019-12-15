#include "Renderer.h"

#include "Core/scene.h"
#include "Core/engine.h"

#include "Components/RenderableManager.h"

#include "Rendering/IblInterface.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/LightingPass.h"
#include "Rendering/Skybox.h"
#include "Rendering/RenderQueue.h"

#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/CommandBufferManager.h"

#include "Threading/ThreadPool.h"

#include "Utility/logger.h"

namespace OmegaEngine
{

Renderer::Renderer(Engine& eng, Scene& scene, VulkanAPI::Swapchain& swapchain) :
	engine(eng), 
	scene(scene), 
	swapchain(swapchain),
    vkDriver(eng.getVkDriver())
{
	// setup ibl if required
	ibl.prepare();
}

Renderer::~Renderer()
{
}

void Renderer::prepare()
{
	// TODO: At the moment only a deffered renderer is supported. Maybe add a
    // forward renderer as well
    for (const RenderStage& stage : deferredStages)
    {
        switch (stage)
        {
            case RenderStage::GBufferFill:
                rStages.emplace_back(std::make_unique<GBufferFillPass>());
                break;
            case RenderStage::LightingPass:
                rStages.emplace_back(std::make_unique<LightingPass>());
                break;
		    case RenderStage::Skybox:
                rStages.emplace_back(std::make_unique<Skybox>());
			    break;
        }
    }

	iblInterface->renderMaps(*vkInterface);
}

void Renderer::beginFrame()
{
    
}

void Renderer::update()
{
   
    
   
}

void Renderer::draw()
{
	vkDriver.beginFrame();

	// optimisation and compilation of the render graph. If nothing has changed since the last frame then this 
	// call will just return.
	rGraph.prepare();
			
	// executes the user-defined callback for all of the passes in-turn. 
	rGraph.execute();

	// finally send to the swap-chain presentation
	vkDriver.submitFrame(swapchain);
}

void Renderer::drawQueueThreaded(VulkanAPI::CmdBuffer& cmdBuffer, RGraphContext& context)
{
    VulkanAPI::CmdBuffer cbSecondary = cmdBuffer.createSecondary();
	auto& queue = scene.renderQueue.getPartition(RenderQueue::Partition::Colour);

    auto thread_draw = [&cbSecondary, &queue, &context](size_t start, size_t end)
    {
		assert(end < queue.size());
		assert(start < end);
		for (size_t idx = start; idx < end; ++idx)
        {
            RenderableQueueInfo& info = queue[idx];
            info.renderFunction(cbSecondary, info.renderableData, context);
        }
    };
    
    // split task up equally per thtread - using a new secondary cmd buffer per thread
    size_t workSize = queue.size();
    ThreadTaskSplitter split{ 0, workSize, thread_draw };
    split.run();
    
	// check all task have finished here before execute?
	cmdBuffer.executeSecondary();
}

}    // namespace OmegaEngine
