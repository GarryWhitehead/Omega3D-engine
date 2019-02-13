#include "Rendering/RenderQueue.h"

namespace OmegaEngine
{

    RenderQueue::RenderQueue(VulkanAPI::Queue& graph, VulkanAPI::Queue& present) :
        graph_queue(graph),
        present_queue(present)
    {
    }

    RenderQueue::~RenderQueue()
    {

    }

    void RenderQueue::submit_graphics()
    {
        
    }
}