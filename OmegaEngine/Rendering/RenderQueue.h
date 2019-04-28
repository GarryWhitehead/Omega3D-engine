#pragma once

#include "Vulkan/Common.h"
#include "Vulkan/CommandBuffer.h"


#include <unordered_map>
#include <vector>

namespace OmegaEngine
{
	// forward declerations
	class ThreadPool;
	struct ProgramState;
	class RenderInterface;
    class ComponentInterface;
	enum class RenderTypes;

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

	enum class RenderStage
	{
		First,
		Light,
		Post
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
        void (*render_function)(void*, VulkanAPI::SecondaryCommandBuffer&, void* renderable_data, RenderInterface*);
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


		static SortKey create_sort_key(RenderStage layer, uint32_t material_id, RenderTypes shader_id);
        void sort_all();

		void submit(VulkanAPI::SecondaryCommandBuffer cmd_buffer,
					RenderInterface* render_interface,
					QueueType type,
					uint32_t start, uint32_t end,
					uint32_t thread_group_size);

		void threaded_dispatch(std::unique_ptr<VulkanAPI::CommandBuffer>& cmd_buffer, RenderInterface* render_interface);

     private:

        // ordered by queue type
        std::unordered_map<QueueType, std::vector<RenderQueueInfo> > render_queues;

    };
}