#include "RenderQueue.h"

#include "Threading/ThreadPool.h"

#include <cassert>

namespace OmegaEngine
{

RenderQueue::RenderQueue()
{
}

RenderQueue::~RenderQueue()
{
}

std::vector<RenderableQueueInfo> RenderQueue::getAll(bool sort)
{
	if (renderables.empty())
	{
		return {};
	}
	if (sort)
	{
		sortAll();
	}

	return getRange(0, renderables.size() - 1);
}

std::vector<RenderableQueueInfo> RenderQueue::getRange(const size_t start, const size_t end, Type type)
{
	// Note:: type not used at present as only one queue type - for future use
	
	assert(end < renderables.size());
	assert(start < end);

	if (renderables.empty())
	{
		return {};
	}

	std::vector<RenderableQueueInfo> output(renderables.begin() + start, renderables.begin() + end);
	return output;
}

void RenderQueue::sortAll()
{
	// TODO : use a radix sort instead
	std::sort(renderables.begin(), renderables.end(), [](const RenderableQueueInfo& lhs, RenderableQueueInfo& rhs) {
		return lhs.sortingKey.u.flags < rhs.sortingKey.u.flags;
	});
}

SortKey RenderQueue::createSortKey(Layer layer, size_t materialId, uint64_t variantId)
{
	SortKey key;

	key.u.s.layerId = (uint64_t)layer;        // layer is the highest priority to group
	key.u.s.textureId = materialId;           // then materials
	key.u.s.shaderId = variantId;             // then shader variant
	key.u.s.depthId = 0;                      // TODO - this is camera-view . mesh_centre - camera-pos

	return key;
}
}    // namespace OmegaEngine
