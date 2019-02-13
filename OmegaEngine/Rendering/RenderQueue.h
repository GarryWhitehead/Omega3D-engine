#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/CommandBuffer.h"

#include <map>
#include <vector>

namespace OmegaEngine
{
	// forward declerations
	class ThreadPool;
	class RenderPipeline;
	class RenderInterface;
	enum class RenderTypes;

	enum class RenderQueueType
	{
		Graphics,
		Present
	};
    
    // all the information required to render 
    struct RenderQueueInfo
    {
		RenderTypes type;

        // render callback function
        void (*render_function)(VulkanAPI::CommandBuffer&, RenderPipeline& , ThreadPool&, uint32_t, uint32_t);
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

		void add_cmd_buffer(VulkanAPI::CommandBuffer& buffer)
		{
			cmd_buffer = buffer;
		}

        void submit(RenderInterface* render_interface);

     private:

        VulkanAPI::CommandBuffer cmd_buffer;

        // ordered by sorting priority
        std::map<uint32_t, std::vector<RenderQueueInfo> > render_queues;

    };
}