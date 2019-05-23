#include "Managers/EventManager.h"

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
					listeners[j].listenerFunction(listeners[j].listenerHandle, *iter->second.events[index]);
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

}