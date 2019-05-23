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
				uint64_t layerId : 4;
				uint64_t shaderId : 12;
				uint64_t textureId : 12;
				uint64_t depthId : 12;
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
        Shadow,
		Opaque,
        Transparent,
		Forward
    };
    
    // all the information required to render 
    struct RenderQueueInfo
    {
        // render callback function
        void (*renderFunction)(void*, VulkanAPI::SecondaryCommandBuffer&, void* renderableData);
		void *renderableHandle;

        // data specific to the renderable - mainly drawing information 
        void *renderableData;

        // the point in the render this will be drawn
        SortKey sortingKey;

		QueueType queueType;
    };

   

    class RenderQueue
    {
     public:

        RenderQueue();
        ~RenderQueue();

        void addRenderableToQueue(RenderQueueInfo& renderInfo)
        {
            renderQueues[renderInfo.queueType].push_back(renderInfo);
        }


		static SortKey createSortKey(RenderStage layer, uint32_t materialId, RenderTypes shaderId);
        void sortAll();

		void submit(VulkanAPI::SecondaryCommandBuffer cmdBuffer,
					QueueType type,
					uint32_t start, uint32_t end,
					uint32_t threadGroupSize);

		void dispatch(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer, QueueType type);
		void threadedDispatch(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer, QueueType type);

     private:

        // ordered by queue type
        std::unordered_map<QueueType, std::vector<RenderQueueInfo> > renderQueues;

    };
}