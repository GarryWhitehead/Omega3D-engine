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

void RenderQueue::reset()
{
	renderables.clear();
	partOffsets.fill(0);
	partSizes.fill(0);
}

void RenderQueue::pushRenderables(std::vector<RenderableQueueInfo>& newRenderables, const Partition part)
{
	partOffsets[part] = renderables.size();
	partSizes[part] = newRenderables.size();
	std::copy(renderables.end(), newRenderables.begin(), newRenderables.end());
}

std::vector<RenderableQueueInfo> RenderQueue::getPartition(const RenderQueue::Partition part)
{
	if (partSizes[part] > 0 || renderables.empty())
	{
		return {};
	}

	auto beginIter = renderables.begin() + partOffsets[part];
	auto endIter = beginIter + partSizes[part];

	std::vector<RenderableQueueInfo> output(beginIter, endIter);
	return output;
}

void RenderQueue::sortPartition(const Partition part)
{
	auto beginIter = renderables.begin() + partOffsets[part];
	auto endIter = beginIter + partSizes[part];

	// TODO : use a radix sort instead
	std::sort(beginIter, endIter, [](const RenderableQueueInfo& lhs, RenderableQueueInfo& rhs) {
		return lhs.sortingKey.u.flags < rhs.sortingKey.u.flags;
	});
}

void RenderQueue::sortAll()
{
	for (size_t i = 0; i < Partition::Count; ++i)
	{
		if (!partSizes[i])
		{
			continue;
		}
		sortPartition(static_cast<const Partition>(i));
	}
}

SortKey RenderQueue::createSortKey(Layer layer, size_t materialId, uint64_t variantId)
{
	SortKey key;

	key.u.s.layerId = (uint64_t)layer;    // layer is the highest priority to group
	key.u.s.textureId = materialId;       // then materials
	key.u.s.shaderId = variantId;         // then shader variant
	key.u.s.depthId = 0;                  // TODO - this is camera-view . mesh_centre - camera-pos

	return key;
}
}    // namespace OmegaEngine
