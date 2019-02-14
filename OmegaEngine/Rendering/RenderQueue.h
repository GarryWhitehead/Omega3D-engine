#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/CommandBuffer.h"

#include <unordered_map>
#include <vector>

namespace OmegaEngine
{
	// forward declerations
	class ThreadPool;
	class RenderPipeline;
	class RenderInterface;
    class ComponentInterface;
	enum class RenderTypes;

	enum class RenderQueueType
	{
		Graphics,
		Present
	};

    enum class QueueType
    {
        Opaque,
        Transparent
    };
    
    // all the information required to render 
    struct RenderQueueInfo
    {
        // render callback function
        void (*render_function)(VulkanAPI::CommandBuffer&, void* renderable_data , std::unique_ptr<ComponentInterface>&, RenderInterface*);

        // data specific to the renderable - mainly drawing information 
        void *renderable_data;

        // the point in the render this will be drawn
        SortingKey sorting_key;
    };

    struct SortKey
    {
        union 
        {
            struct 
            {
                // in order of sorting importance
                uint64_t layer_id : 4;
                uint64_t shader_id : 12;
                uint64_t texture_id : 12;
                uint64_t depth_id : 12;
            } s;

            uint64_t flags;

        } u;

        float depth;    // for transparency;
    };

    class RenderQueue
    {
     public:

        RenderQueue();
        ~RenderQueue();

        void add_to_queue(RenderQueueInfo& render_info, QueueType type)
        {
            render_queues[type].push_back(render_info);
        }

		void add_cmd_buffer(VulkanAPI::CommandBuffer& buffer)
		{
			cmd_buffer = buffer;
		}

        static SortKey create_sort_key(QueueType type, RendererType renderer_type, uint32_t material_id, RenderTypes shader_id);
        void sort_all();
        void submit(RenderInterface* render_interface);

     private:

        VulkanAPI::CommandBuffer cmd_buffer;

        // ordered by queue type
        std::unordered_map<QueueType, std::vector<RenderQueueInfo> > render_queues;

    };
}