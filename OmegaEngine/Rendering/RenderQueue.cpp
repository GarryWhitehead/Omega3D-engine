#include "RenderQueue.h"
#include "Threading/ThreadPool.h"
#include "Rendering/RenderInterface.h"

namespace OmegaEngine
{

    RenderQueue::RenderQueue() 
    {
    }

    RenderQueue::~RenderQueue()
    {

    }

    void RenderQueue::submit(VulkanAPI::SecondaryCommandBuffer cmd_buffer,
							 RenderInterface* render_interface, 
                             QueueType type, 
                             uint32_t start, uint32_t end, 
							 uint32_t thread_group_size)
    {
		
        // start the secondary command buffer recording - using one cmd buffer and pool per thread 
		cmd_buffer.begin();

        for (uint32_t i = start; i < end; i++) {
            
            RenderQueueInfo& queue_info = render_queues[type][i];
            queue_info.render_function(queue_info.renderable_handle, cmd_buffer, queue_info.renderable_data, render_interface);
        }

		cmd_buffer.end();
    }


    void RenderQueue::threaded_dispatch(VulkanAPI::CommandBuffer& cmd_buffer, RenderInterface* render_interface)
    {
		uint32_t num_threads = std::thread::hardware_concurrency();
		ThreadPool thread_pool(num_threads);

		// create the cmd pools and secondary buffers for each stage
		cmd_buffer.create_secondary(num_threads);

		// render by queue type - opaque, lighting and then transparent meshes
        for (auto queue : render_queues) {            

			// TODO: threading is a bit crude at the mo - find a better way of splitting this up - maybe based on materials types, etc.
			uint32_t thread_group_size = queue.second.size() / num_threads;
			thread_group_size = thread_group_size < 1 ? 1 : thread_group_size;

            for (uint32_t i = 0, thread = 0; i < queue.second.size(); i += thread_group_size, ++thread) {

                VulkanAPI::SecondaryCommandBuffer sec_cmd_buffer = cmd_buffer.get_secondary(thread);

                // if we have no more threads left, then draw every thing that is remaining
                if (i + 1 >= num_threads) {
            
                    thread_pool.submitTask([=]() {
				        submit(sec_cmd_buffer, render_interface, queue.first, i, queue.second.size(), thread_group_size);
			            });
                    break;
                }

                thread_pool.submitTask([=]() {
				    submit(sec_cmd_buffer, render_interface, queue.first, i, i + thread_group_size, thread_group_size);
			        });
		    } 
        }

		// check that all threads are finshed before executing the cmd buffers
		thread_pool.wait_for_all();

		// execute the recorded secondary command buffers - only for those threads we have actually used
		cmd_buffer.execute_secondary_commands(2);

        // TODO:: maybe optional? if the renderable data is hasn't changed then we can reuse the queue
		render_queues.clear();
	}

    void RenderQueue::draw(LayerType layer, std::vector<RenderQueueInfo>& queue)
    {
        // now draw all renderables to the pass - start by begining the renderpass 
        cmd_buffer.create_primary();
        vk::RenderPassBeginInfo begin_info = renderpass.get_begin_info(vk::ClearColorValue(render_config.general.background_col));
        cmd_buffer.begin_renderpass(begin_info, true);

        // now draw everything in the queue - TODO: add all renderpasses to the queue (offscreen stuff, etc.)
        render_queue->threaded_dispatch(cmd_buffer, this);

        // end the primary pass and buffer
        cmd_buffer.end_pass();
        cmd_buffer.end();
    }

    void RenderQueue::draw_all(bool sort_queue)
    {
         // sort by the set order - layer, shader, material and depth
        if (sort_queue) {
            sort_all();
        }

        uint32_t layer_count = 0;
        for (auto& queue : render_queue) {
            
            // do in order - check it exsists first
            if (render_queue.find(layer_count) != render_queue.end()) {
                
                // first sort by queue type i.e transparency and opaque
                std::vector<RenderQueueInfo> transparent_render;
                std::vector<RenderQueueInfo> opaque_render;

                for (auto& queue_obj : queue.second) {

                    if (queue_obj.queue_type == QueueType::Opaque) {
                        opaque_render.push_back(queue_obj);
                    }
                    else if(queue_obj.queue_type == QueueType::Transparent) {
                        transparent_render.push_back(queue_obj);
                    }
                    else {
                        continue;
                    }
                }

            }
            
            
        }
    }

    void RenderQueue::sort_all()
    {
        for (auto queue : render_queues) {
            
            // TODO : use a radix sort instead
            std::sort(queue.second.begin(), queue.second.end(), [](const RenderQueueInfo& a, RenderQueueInfo& b) {
                return a.sorting_key.u.flags < b.sorting_key.u.flags;
            });
        }
    }


     SortKey RenderQueue::create_sort_key(RenderStage layer, uint32_t material_id, RenderTypes shader_id)
     {
         SortKey key;

         key.u.s.layer_id = (uint64_t)layer;	// layer is the highest priority to group
         key.u.s.texture_id = material_id;				// then materials
         key.u.s.shader_id = (uint64_t)shader_id;		// then shader
         key.u.s.depth_id = 0;							// TODO - this is camera-view . mesh_centre - camera-pos

         return key;
     }
}