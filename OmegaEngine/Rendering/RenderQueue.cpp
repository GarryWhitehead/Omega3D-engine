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

    void RenderQueue::submit(RenderInterface* render_interface)
    {
		uint32_t num_threads = std::thread::hardware_concurrency();
		uint32_t threads_per_group = VULKAN_THREADED_GROUP_SIZE / num_threads;

		ThreadPool thread_pool(num_threads);

		// render by priority key
        for (auto queue : render_queues) {

            for (auto& renderable : queue.second) {

                renderable.render_function(cmd_buffer, render_interface->get_render_pipeline(renderable.type), thread_pool, num_threads, threads_per_group);

            }
        }

        // check that all threads are finshed before executing the cmd buffers
        cmd_buffer.secondary_execute_commands();
		cmd_buffer.end();

        // TODO:: maybe optional? if the renderable data is hasn't changed then we can reuse the queue
        render_queues.clear();
    }
}