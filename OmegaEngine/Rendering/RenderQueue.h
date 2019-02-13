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
        RenderQueueType type;
        vk::CommandBuffer cmd_buffer;
        vk::Semaphore wait_semaphore;
        vk::Semaphore signal_semaphore;
        vk::PipelineStageFlagBits stage;
    };

    class RenderQueue
    {
     public:

        RenderQueue(VulkanAPI::Queue& graph, VulkanAPI::Queue& present);
        ~RenderQueue();

        void submit_graphics();

     private:

        std::queue<RenderQueueInfo> render_queue;

        VulkanAPI::Queue graph_queue;
        VulkanAPI::Queue present_queue;
    };
}