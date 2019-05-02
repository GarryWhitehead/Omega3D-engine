#include "ObjectRenderer.h"
#include "Rendering/RenderQueue.h"
#include "Rendering/RenderInterface.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Skybox.h"

namespace OmegaEngine
{

	namespace Rendering
	{
		void render_objects(std::unique_ptr<RenderQueue>& render_queue, 
							RenderConfig& render_config, 
							VulkanAPI::RenderPass& renderpass,
							std::vector<RenderInterface::RenderableInfo>& renderables,
							std::unique_ptr<VulkanAPI::CommandBuffer>& cmd_buffer)
		{
			RenderQueueInfo queue_info;

			for (auto& info : renderables) {

				switch (info.renderable->get_type()) {
				case RenderTypes::SkinnedMesh:
				case RenderTypes::StaticMesh: {
					queue_info.renderable_handle = info.renderable->get_handle();
					queue_info.render_function = get_member_render_function<void, RenderableMesh, &RenderableMesh::render>;
					break;
				}
				case RenderTypes::Skybox: {
					queue_info.renderable_handle = info.renderable->get_handle();
					queue_info.render_function = get_member_render_function<void, RenderableSkybox, &RenderableSkybox::render>;
					break;
				}
				}

				queue_info.renderable_data = info.renderable->get_instance_data();
				queue_info.sorting_key = info.renderable->get_sort_key();
				queue_info.queue_type = info.renderable->get_queue_type();

				render_queue.add_to_queue(queue_info);
			}

			// sort by the set order - layer, shader, material and depth
			render_queue.sort_all();

			// now draw all renderables to the pass - start by begining the renderpass 
			cmd_buffer->create_primary();
			vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
			cmd_buffer->begin_renderpass(begin_info, true);

			// now draw everything in the queue - TODO: add all renderpasses to the queue (offscreen stuff, etc.)
			render_queue.threaded_dispatch(cmd_buffer);

			// end the primary pass and buffer
			cmd_buffer->end_pass();
			cmd_buffer->end();
		}
	}
}