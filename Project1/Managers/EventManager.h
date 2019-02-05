#pragma once
#include <unordered_map>
#include <queue>
#include <functional>
#include <memory>

#include "Utility/GeneralUtil.h"

namespace OmegaEngine
{

	class Event
	{
	public:

		bool shouldDelete = true;
	};

	struct Listener
	{
		std::function<void(Event&)> func;
		void* handle;
	};

	class EventManager
	{
	public:

		struct EventData
		{
			std::vector<Listener*> listeners;
			std::vector<std::unique_ptr<Event> > events;

		};

		EventManager();

		template <typename T, typename EventType>
		void registerListener(T* l, std::function<void(Event&)> func)
		{
			// generate unique id for event type
			uint64_t type = Util::event_type_id<EventType>();
			auto &data = eventQueue[type];

			data.listeners.push_pack({ func, &l });
		}

		template <typename EventType, typename... Args>
		void addQueueEvent(EventType event, Args&&... args)
		{
			uint64_t type = Util::event_type_id<EventType>();
			auto iter = eventQueue.find(type);

			// does the event type exsist?
			if (iter != eventQueue.end()) {
				auto e = std::unique_ptr<Event>(new EventType(std::forward<Args>(args)...));
				iter->second.events.emplace(std::move(e));
			}
		}

		template <typename EventType>
		void instantNotification(EventType e)
		{
			// find all listeners that are registered with this event type
			uint64_t type = Util::event_type_id<EventType>();

			if (iter != eventQueue.end()) {
				EventData data = iter->second;
				for (auto& listener : data.listeners) {

					// call registered member function with event
					listener->func(e);
				}
			}
		}

		void notifyQueued();


	private:

		std::unordered_map<uint64_t, EventData> eventQueue;
	};

}

