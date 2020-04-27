/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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

void RenderQueue::resetAll()
{
    for (size_t i = 0; i < RenderQueue::Type::Count; ++i)
    {
        renderables[i].clear();
    }
}

void RenderQueue::pushRenderables(
    std::vector<RenderableQueueInfo>& newRenderables, const RenderQueue::Type type)
{
    size_t newSize = newRenderables.size();
    std::vector<RenderableQueueInfo>& rQueue = renderables[type];
    rQueue.clear();
    rQueue.resize(newSize);

    std::copy(newRenderables.begin(), newRenderables.end(), rQueue.begin());
}

std::vector<RenderableQueueInfo> RenderQueue::getQueue(const RenderQueue::Type type)
{
    return renderables[type];
}

void RenderQueue::sortQueue(const RenderQueue::Type type)
{
    std::vector<RenderableQueueInfo>& rQueue = renderables[type];
    if (rQueue.empty())
    {
        return;
    }

    // TODO : use a radix sort instead
    std::sort(
        rQueue.begin(), rQueue.end(), [](const RenderableQueueInfo& lhs, RenderableQueueInfo& rhs) {
            return lhs.sortingKey.u.flags < rhs.sortingKey.u.flags;
        });
}

void RenderQueue::sortAll()
{
    for (size_t i = 0; i < RenderQueue::Type::Count; ++i)
    {
        sortQueue(static_cast<RenderQueue::Type>(i));
    }
}

SortKey RenderQueue::createSortKey(Layer layer, size_t materialId, uint64_t variantId)
{
    SortKey key;

    key.u.s.layerId = (uint64_t) layer; // layer is the highest priority to group
    key.u.s.textureId = materialId; // then materials
    key.u.s.shaderId = variantId; // then shader variant
    key.u.s.depthId = 0; // TODO - this is camera-view . mesh_centre - camera-pos

    return key;
}
} // namespace OmegaEngine
