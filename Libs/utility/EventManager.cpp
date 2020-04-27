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

#include "EventManager.h"

namespace OmegaEngine
{
EventManager::EventManager()
{
}

EventManager::~EventManager()
{
    if (!eventQueue.empty())
    {
        for (auto& queue : eventQueue)
        {
            if (!queue.second.events.empty())
            {
                for (auto& event : queue.second.events)
                {
                    delete event;
                }
            }
        }
    }
}

void EventManager::notifyQueued()
{
    auto iter = eventQueue.begin();

    while (iter != eventQueue.end())
    {
        auto& listeners = iter->second.listeners;

        uint32_t index = 0;
        while (index < iter->second.events.size())
        {
            for (uint32_t j = 0; j < listeners.size(); ++j)
            {
                listeners[j].listenerFunction(
                    listeners[j].listenerHandle, *iter->second.events[index]);
            }

            if (iter->second.events[index]->shouldDelete)
            {
                iter->second.events.erase(iter->second.events.begin() + index);
            }
            else
            {
                index++;
            }
        }
        iter++;
    }
}

} // namespace OmegaEngine
