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
                             uint32_t thread)
    {
		
        // create a secondary command buffer for each thread. This also creates a thread-specific command pool 
		cmd_buffer.begin_secondary(thread);

        for (uint32_t i = start; i < end; i += thread_group_size) {
            
            RenderQueueInfo& queue_info = render_queues[type][i];
            queue_info.render_function(cmd_buffer, queue_info.renderable_data, render_interface, thread);
        }
    }


    void RenderQueue::threaded_dispatch(RenderInterface* render_interface, std::unique_ptr<ComponentInterface>& component_interface)
    {
        uint32_t num_threads = std::thread::hardware_concurrency();
		uint32_t thread_group_size = VULKAN_THREADED_GROUP_SIZE / num_threads;

		ThreadPool thread_pool(num_threads);

        uint32_t thread_count = 0;

		// render by queue type - opaque, lighting and then transparent meshes
        for (auto queue : render_queues) {            
            
            for (uint32_t i = 0; i < queue.second.size(); i += thread_group_size) {

                // if we have no more threads left, then draw every mesh that is remaining
                if (i + 1 >= num_threads) {
            
                    thread_pool.submitTask([&]() {
						submit(render_interface, queue.first, i, queue.second.size(), thread_count);
                    });
                    break;
                }

                thread_pool.submitTask([&]() {
					submit(render_interface, queue.first, i, i + thread_group_size, thread_count);
                });

                ++thread_count;
		    }   

            // check that all threads are finshed before executing the cmd buffers
            thread_pool.wait_for_all();

            cmd_buffer.secondary_execute_commands();
            cmd_buffer.end();
        }

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


     SortKey RenderQueue::create_sort_key(QueueType queue_type, RendererType renderer_type, uint32_t material_id, RenderTypes shader_id)
     {
         SortKey key;

         key.u.s.layer_id = (uint64_t)renderer_type;	// layer is the highest priority to group
         key.u.s.texture_id = material_id;				// then materials
         key.u.s.shader_id = (uint64_t)shader_id;		// then shader
         key.u.s.depth_id = 0;							// TODO - this is camera-view . mesh_centre - camera-pos

         return key;
     }
}