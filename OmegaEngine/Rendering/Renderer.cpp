#include "Renderer.h"

#include "Core/scene.h"
#include "Core/engine.h"

#include "Components/RenderableManger.h"

#include "Rendering/IblInterface.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/LightingPass.h"
#include "Rendering/Skybox.h"
#include "Rendering/RenderQueue.h"

#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/CmdBuffer.h"
#include "VulkanAPI/Managers/CommandBufferManager.h"

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
    // render queues are built each frame
    renderQueue.reset();
}

void Renderer::update()
{
    // uopdate the render queue with visible renderables derived from the scene
    // NOTE: makes sure that you have updated the scene before calling this!
    RenderableManager& manager = engine.getRendManager();
    
    for (const Renderable& rend : manager.renderables)
    {
        RenderableQueueInfo queueInfo;
        // we use the renderable data as it is, rather than waste time copying everything into another struct.
        // This method does mean that it is imperative that the data isnt destroyed until the beginning of the next
        // frame ad that the data isn't written too - we aren't using guards though this might be required.
        queueInfo.renderableData = &rend;
        queueInfo.renderableHandle = this;
        queueInfo.renderFunction = GBufferFillPass::drawCallback;
        queueInfo.sortingKey = RenderQueue::createSortKey(Layer::Default, rend.materialId, rend.variant);
        renderQueue.push(queueInfo);
    }
}

void Renderer::draw()
{
	// update the unifom buffers on the backend
	vkDriver.updateUbo();

	// optimisation and compilation of the render graph. If nothing has changed since the last frame then this 
	// call will just return.
	rGraph.prepare();
			
	// executes the user-defined callback for all of the passes in-turn. 
	rGraph.execute();

	// finally send to the swap-chain presentation
	vkDriver.submitFrame(swapchain);
}

void Renderer::drawQueueThreaded(VulkanAPI::CmdBuffer& cmdBuffer, RenderQueue::Type type, RGraphContext& context)
{
    VulkanAPI::CmdBuffer cbSecondary = cmdBuffer.createSecondary();
	auto& queue = renderQueue.renderables;

    auto thread_draw = [&cbSecondary, &queue, &type, &context](size_t start, size_t end)
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
    size_t workSize = renderQueue.renderables.size();
    ThreadTaskSplitter split{ 0, workSize, thread_draw };
    split.run();
    
	// check all task have finished here before execute?
	cmdBuffer.executeSecondary();
}

}    // namespace OmegaEngine
