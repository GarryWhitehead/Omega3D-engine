#include "RenderQueue.h"
#include "Rendering/RenderInterface.h"
#include "Threading/ThreadPool.h"

namespace OmegaEngine
{

RenderQueue::RenderQueue()
{
}

RenderQueue::~RenderQueue()
{
}

void RenderQueue::submit(VulkanAPI::SecondaryCommandBuffer cmdBuffer, QueueType type, uint32_t start, uint32_t end,
                         uint32_t threadGroupSize)
{

	// start the secondary command buffer recording - using one cmd buffer and pool per thread
	cmdBuffer.begin();

	for (uint32_t i = start; i < end; i++)
	{

		RenderQueueInfo& queueInfo = renderQueues[type][i];
		queueInfo.renderFunction(queueInfo.renderableHandle, cmdBuffer, queueInfo.renderableData);
	}

	cmdBuffer.end();
}

void RenderQueue::dispatch(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer, QueueType type)
{
	// render by queue type
	auto& queue = renderQueues[type];

	cmdBuffer->createSecondary(1);
	VulkanAPI::SecondaryCommandBuffer secondaryCmdBuffer = cmdBuffer->getSecondary(0);

	secondaryCmdBuffer.begin();

	for (uint32_t i = 0; i < queue.size(); ++i)
	{

		RenderQueueInfo& queueInfo = queue[i];
		queueInfo.renderFunction(queueInfo.renderableHandle, secondaryCmdBuffer, queueInfo.renderableData);
	}

	secondaryCmdBuffer.end();

	// execute the recorded secondary command buffers
	cmdBuffer->executeSecondaryCommands(1);
}

void RenderQueue::threadedDispatch(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer, QueueType type)
{
	// submits the draw calls in the range specified for items in the queue
	auto renderFunc = [](VulkanAPI::SecondaryCommandBuffer& secBuffer, std::vector<RenderQueueInfo>& renderQueue,
	                     const uint32_t start, const uint32_t end, const uint32_t groupSize) -> void {
		// start the secondary command buffer recording - using one cmd buffer and pool per thread
		secBuffer.begin();

		for (uint32_t i = start; i < end; i++)
		{
			renderQueue[i].renderFunction(renderQueue[i].renderableHandle, secBuffer, renderQueue[i].renderableData);
		}

		secBuffer.end();
	};

	uint32_t threadCount = std::thread::hardware_concurrency();
	ThreadPool threadPool(threadCount);

	// create the cmd pools and secondary buffers for each stage
	cmdBuffer->createSecondary(threadCount);

	// render by queue type
	auto& queue = renderQueues[type];

	uint32_t threadsUsed = 0;

	// TODO: threading is a bit crude at the mo - find a better way of splitting this up - maybe based on materials types, etc.
	uint32_t threadGroupSize = static_cast<uint32_t>(queue.size() / threadCount);
	threadGroupSize = threadGroupSize < 1 ? 1 : threadGroupSize;

	for (uint32_t i = 0, thread = 0; i < queue.size(); i += threadGroupSize, ++thread)
	{
		auto& secondaryCmdBuffer = cmdBuffer->getSecondary(thread);

		// if we have no more threads left, then draw every thing that is remaining
		if (i + 1 >= threadCount)
		{

			auto fut = threadPool.submitTask(renderFunc, std::ref(secondaryCmdBuffer), std::ref(queue), i,
			                                 static_cast<uint32_t>(queue.size()), threadGroupSize);
			break;
		}

		auto fut = threadPool.submitTask(renderFunc, std::ref(secondaryCmdBuffer), std::ref(queue), i,
		                                 i + threadGroupSize, threadGroupSize);

		++threadsUsed;
	}

	// check that all threads are finshed before executing the cmd buffers
	//threadPool.waitForAll();

	// execute the recorded secondary command buffers - only for those threads we have actually used
	cmdBuffer->executeSecondaryCommands(threadsUsed);
}

void RenderQueue::sortAll()
{
	for (auto queue : renderQueues)
	{

		// TODO : use a radix sort instead
		std::sort(queue.second.begin(), queue.second.end(), [](const RenderQueueInfo& a, RenderQueueInfo& b) {
			return a.sortingKey.u.flags < b.sortingKey.u.flags;
		});
	}
}

SortKey RenderQueue::createSortKey(RenderStage layer, uint32_t materialId, RenderTypes shaderId)
{
	SortKey key;

	key.u.s.layerId = (uint64_t)layer;        // layer is the highest priority to group
	key.u.s.textureId = materialId;           // then materials
	key.u.s.shaderId = (uint64_t)shaderId;    // then shader
	key.u.s.depthId = 0;                      // TODO - this is camera-view . mesh_centre - camera-pos

	return key;
}
}    // namespace OmegaEngine
