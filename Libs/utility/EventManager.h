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

#pragma once
#include "GeneralUtil.h"

#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

namespace OmegaEngine
{
class Event;

// function to get around that event structs are inherited, so get the function ptr from the derived
// class
template <
    typename FuncReturn,
    typename T,
    typename EventType,
    FuncReturn (T::*callback)(EventType& event)>
FuncReturn get_member_function(void* object, Event& event)
{
    return (reinterpret_cast<T*>(object)->*callback)(reinterpret_cast<EventType&>(event));
}

class Event
{
public:
    Event() = default;
    virtual ~Event() = default;

    bool shouldDelete = true;
};

struct Listener
{
    void (*listenerFunction)(void*, Event&);
    void* listenerHandle;
};

class EventManager
{
public:
    struct EventData
    {
        std::vector<Listener> listeners;
        std::vector<Event*> events;
    };

    EventManager();
    ~EventManager();

    template <typename T, typename EventType, void (T::*listenerFunction)(EventType&)>
    void registerListener(T* listener)
    {
        // generate unique id for event type
        uint64_t type = Util::TypeId<EventType>::id();
        if (eventQueue.find(type) == eventQueue.end())
        {
            EventData eventData;
            eventData.listeners.push_back(
                {get_member_function<void, T, EventType, listenerFunction>, listener});
            eventQueue[type] = eventData;
        }
        else
        {
            eventQueue[type].listeners.push_back(
                {get_member_function<void, T, EventType, listenerFunction>, listener});
        }
    }

    template <typename EventType, typename... Args>
    void addQueueEvent(Args&&... args)
    {
        uint64_t type = Util::TypeId<EventType>::id();
        auto iter = eventQueue.find(type);

        // does the event type exsist?
        if (iter != eventQueue.end())
        {
            iter->second.events.push_back(new EventType(std::forward<Args>(args)...));
        }
    }

    template <typename EventType>
    void instantNotification(EventType& event)
    {
        // find all listeners that are registered with this event type
        uint64_t type = Util::TypeId<EventType>::id();
        auto iter = eventQueue.find(type);

        if (iter != eventQueue.end())
        {
            EventData data = iter->second;
            for (auto& listener : data.listeners)
            {
                // call registered member function with event
                listener.listenerFunction(listener.listenerHandle, event);
            }
        }
    }

    void notifyQueued();

private:
    std::unordered_map<uint64_t, EventData> eventQueue;
};

} // namespace OmegaEngine
