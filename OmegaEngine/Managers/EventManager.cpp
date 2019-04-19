#include "Managers/EventManager.h"

namespace OmegaEngine
{
	EventManager::EventManager()
	{

	}

	EventManager::~EventManager()
	{

	}

	void EventManager::notifyQueued()
	{
		auto iter = eventQueue.begin();

		while (iter != eventQueue.end()) {

			auto& listeners = iter->second.listeners;

			uint32_t index = 0;
			while (index < iter->second.events.size()) {

				for (uint32_t j = 0; j < listeners.size(); ++j) {

					listeners[j].listener_func(listeners[j].listener_handle, *iter->second.events[index]);

				}

				if (iter->second.events[index]->shouldDelete) {
					
					iter->second.events.erase(iter->second.events.begin() + index);
				}
				else {
					index++;
				}
			}
			iter++;
		}
	}

}