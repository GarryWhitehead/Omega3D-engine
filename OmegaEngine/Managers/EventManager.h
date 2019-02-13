#pragma once
#include "Utility/GeneralUtil.h"

#include <unordered_map>
#include <queue>
#include <functional>
#include <memory>
#include <vector>

namespace OmegaEngine
{
	class Event;

	// function to get around that event structs are inherited, so get the function ptr from the derived class
	template <typename FuncReturn, typename T, typename EventType, FuncReturn(T::*callback)(EventType& event)>
	FuncReturn get_member_function(void *object, Event& event)
	{
		return (reinterpret_cast<T*>(object)->*callback)(reinterpret_cast<EventType&>(event));
	}

	class Event
	{
	public:
	
		virtual ~Event() = default;

		bool shouldDelete = true;
	};

	struct Listener
	{
		void (*listener_func)(void *object, Event& event);
		void* listener_handle;
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

		template <typename T, typename EventType, void (T::*listener_func)(EventType&)>
		void registerListener(T* listener)
		{
			// generate unique id for event type
			uint64_t type = Util::event_type_id<EventType>();
			auto &data = eventQueue[type];

			data.listeners.push_back({ get_member_function<void, T, EventType, listener_func>, listener });
		}

		template <typename EventType, typename... Args>
		void addQueueEvent(EventType event, Args&&... args)
		{
			uint64_t type = Util::event_type_id<EventType>();
			auto iter = eventQueue.find(type);

			// does the event type exsist?
			if (iter != eventQueue.end()) {
				EventType* e = new EventType(std::forward<Args>(args)...);
				iter->second.events.push_back(e);
			}
		}

		template <typename EventType>
		void instantNotification(EventType event)
		{
			// find all listeners that are registered with this event type
			uint64_t type = Util::event_type_id<EventType>();
			auto iter = eventQueue.find(type);

			if (iter != eventQueue.end()) {
				EventData data = iter->second;
				for (auto& listener : data.listeners) {

					// call registered member function with event
					listener.listener_func(listener.listener_handle, event);
				}
			}
		}

		void notifyQueued();


	private:

		std::unordered_map<uint64_t, EventData> eventQueue;
	};

}
