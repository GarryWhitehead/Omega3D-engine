#include "RenderCommon.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Skybox.h"

namespace OmegaEngine
{

	namespace Rendering
	{
		void renderObjects(std::unique_ptr<RenderQueue>& renderQueue, 
							VulkanAPI::RenderPass& renderpass,
							std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer,
							QueueType type,
							RenderConfig& renderConfig)
		{

			// sort by the set order - layer, shader, material and depth
			if (renderConfig.general.sort_renderQueue)
			{
				renderQueue->sortAll();
			}

			// now draw all renderables to the pass - start by begining the renderpass 
			cmdBuffer->createPrimary();
			vk::RenderPassBeginInfo beginInfo = renderpass.getBeginInfo(vk::ClearColorValue(renderConfig.general.backgroundColour));
			cmdBuffer->beginRenderpass(beginInfo, true);

			// now draw everything in the designated queue 
			renderQueue->threadedDispatch(cmdBuffer, type);

			// end the primary pass and buffer
			cmdBuffer->endRenderpass();
			cmdBuffer->end();
		}
	}
}