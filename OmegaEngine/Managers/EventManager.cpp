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

			for (auto& event : iter->second.events) {

				for (uint32_t i = 0; i < listeners.size(); ++i) {

					listeners[i].listener_func(listeners[i].listener_handle, *event);

				}

				if (event->shouldDelete) {
					
				}
			}
			iter++;
		}
	}

}