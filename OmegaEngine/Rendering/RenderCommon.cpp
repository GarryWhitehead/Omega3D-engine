#include "RenderCommon.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Skybox.h"

namespace OmegaEngine
{

	namespace Rendering
	{
		void render_objects(std::unique_ptr<RenderQueue>& render_queue, 
							VulkanAPI::RenderPass& renderpass,
							std::unique_ptr<VulkanAPI::CommandBuffer>& cmd_buffer,
							QueueType type,
							RenderConfig& render_config)
		{

			// sort by the set order - layer, shader, material and depth
			if (render_config.general.sort_render_queue)
			{
				render_queue->sort_all();
			}

			// now draw all renderables to the pass - start by begining the renderpass 
			cmd_buffer->create_primary();
			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
			cmd_buffer->begin_renderpass(begin_info, true);

			// now draw everything in the designated queue 
			render_queue->threaded_dispatch(cmd_buffer, type);

			// end the primary pass and buffer
			cmd_buffer->end_pass();
			cmd_buffer->end();
		}
	}
}