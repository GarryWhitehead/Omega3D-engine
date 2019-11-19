#include "Renderer.h"

#include "Core/engine.h"

#include "Components/CameraManager.h"
#include "Components/LightManager.h"

#include "PostProcess/PostProcessInterface.h"

#include "Rendering/IblInterface.h"
#include "Rendering/GBufferFillPass.h"
#include "Rendering/LightingPass.h"
#include "Rendering/Skybox.h"

#include "VulkanAPI/VkDriver.h"

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
	cmdBufferManager->submitFrame(vkInterface->getSwapchain());
}

}    // namespace OmegaEngine
