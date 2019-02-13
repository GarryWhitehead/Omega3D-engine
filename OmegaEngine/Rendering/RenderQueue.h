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
        VulkanAPI::CommandBuffer cmd_buffer;
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

        void add_to_queue(RenderQueueInfo render_info, uint32_t priority_key);
        void submit();

     private:

        // ordered by sorting priority
        std::map<uint32_t, RenderQueueInfo> render_queue;

    };
}