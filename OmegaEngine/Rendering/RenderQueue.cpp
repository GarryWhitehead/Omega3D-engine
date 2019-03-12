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

    void RenderQueue::submit(RenderInterface* render_interface, 
                             QueueType type, 
                             uint32_t start, uint32_t end, 
                             uint32_t thread,
							 uint32_t thread_group_size)
    {
		
        // create a secondary command buffer for each thread. This also creates a thread-specific command pool 
		cmd_buffer.begin_secondary(thread);

        for (uint32_t i = start; i < end; i++) {
            
            RenderQueueInfo& queue_info = render_queues[type][i];
            queue_info.render_function(queue_info.renderable_handle, cmd_buffer, queue_info.renderable_data, render_interface, thread);
        }
    }


    void RenderQueue::threaded_dispatch(RenderInterface* render_interface)
    {
        uint32_t num_threads = std::thread::hardware_concurrency();
		
		ThreadPool thread_pool(num_threads);
        uint32_t thread_count = 0;

		// create the cmd pools and secondary buffers for each stage
		cmd_buffer.create_secondary(num_threads, true);


		// render by queue type - opaque, lighting and then transparent meshes
        for (auto queue : render_queues) {            

			// TODO: threading is a bit crude at the mo - find a better way of splitting this up - maybe based on materials types, etc.
			uint32_t thread_group_size = queue.second.size() / num_threads;
			thread_group_size = thread_group_size < 1 ? 1 : thread_group_size;

			auto drawRenderable = [&](const int startIndex) ->void {
				submit(render_interface, queue.first, startIndex, startIndex + thread_group_size, thread_count, thread_group_size);
			};

            for (uint32_t i = 0; i < queue.second.size(); i += thread_group_size) {

                // if we have no more threads left, then draw every thing that is remaining
                if (i + 1 >= num_threads) {
            
                    thread_pool.submitTask(std::bind(drawRenderable, i));
                    break;
                }

                thread_pool.submitTask(std::bind(drawRenderable, i));

                ++thread_count;
		    }   

            // check that all threads are finshed before executing the cmd buffers
            thread_pool.wait_for_all();

            cmd_buffer.secondary_execute_commands();    
        }

		cmd_buffer.end_pass();
		cmd_buffer.end();

        // TODO:: maybe optional? if the renderable data is hasn't changed then we can reuse the queue
		render_queues.clear();
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