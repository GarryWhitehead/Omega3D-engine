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
	enum class RenderStage;

	enum class RenderQueueType
	{
		Graphics,
		Present
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

    enum class QueueType
    {
        Opaque,
        Transparent
    };
    
    // all the information required to render 
    struct RenderQueueInfo
    {
        // render callback function
        void (*render_function)(void*, VulkanAPI::CommandBuffer&, void* renderable_data, RenderInterface*, uint32_t);
		void *renderable_handle;

        // data specific to the renderable - mainly drawing information 
        void *renderable_data;

        // the point in the render this will be drawn
        SortKey sorting_key;

		QueueType queue_type;
    };

   

    class RenderQueue
    {
     public:

        RenderQueue();
        ~RenderQueue();

        void add_to_queue(RenderQueueInfo& render_info)
        {
            render_queues[render_info.queue_type].push_back(render_info);
        }

		void add_cmd_buffer(VulkanAPI::CommandBuffer& buffer)
		{
			cmd_buffer = buffer;
		}

		static SortKey create_sort_key(RenderStage layer, uint32_t material_id, RenderTypes shader_id);
        void sort_all();

        void submit(RenderInterface* render_interface,
					QueueType type,
					uint32_t start, uint32_t end,
					uint32_t thread,
					uint32_t thread_group_size);

		void threaded_dispatch(RenderInterface* render_interface, std::unique_ptr<ComponentInterface>& component_interface);

     private:

        VulkanAPI::CommandBuffer cmd_buffer;

        // ordered by queue type
        std::unordered_map<QueueType, std::vector<RenderQueueInfo> > render_queues;

    };
}