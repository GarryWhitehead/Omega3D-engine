#include <queue>

#include "Vulkan/Common.h"

namespace OmegaEngine
{
    enum class RenderQueueType
    {
        Graphics,
        Present
    }
    
    // all the information required to render 
    struct RenderQueueInfo
    {
        ThreadPool* thread_pool;
        uint32_t num_threads;
        uint32_t threads_per_group;

        // render callback function
        (void* render_function)(VulkanAAPI::CommandBuffer&, RenderPipeline& , ThreadPool*, uint32_t, uint32_t);
    };

    class RenderQueue
    {
     public:

        RenderQueue();
        ~RenderQueue();

        void add_to_queue(RenderQueueInfo& render_info, uint32_t priority_key)
        {
            render_queues[priority_key].push_back(render_info);
        }

        void submit(RenderInterface* interface);

     private:

        VulkanAPI::CommandBuffer cmd_buffer;

        // ordered by sorting priority
        std::map<uint32_t, std::vector<RenderQueueInfo> > render_queues;

    };
}