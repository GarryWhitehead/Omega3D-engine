#include "Rendering/RenderQueue.h"

namespace OmegaEngine
{

    RenderQueue::RenderQueue() 
    {
    }

    RenderQueue::~RenderQueue()
    {

    }

    void RenderQueue::submit(RenderInterface* interface)
    {
        // render by priority key
        for (auto queue : render_queues) {

            for (auto& renderable : queue.second) {

                renderable.render_function(cmd_buffer, interface->get_render_pipeline(renderable.type), renderable.thread_pool, renderable.num_threads, renderable.threads_per_group);

            }
        }

        // check that all threads are finshed before executing the cmd buffers
        cmd_buffer.secondary_execute_commands();
		thread_pool.wait_for_all();

		cmd_buffer.end();

        // TODO:: maybe optional? if the renderable data is hasn't changed then we can reuse the queue
        render_queues.clear();
    }
}