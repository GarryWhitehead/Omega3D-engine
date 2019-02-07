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

			auto& events = iter->second.events;
			auto& listeners = iter->second.listeners;

			for (auto& event : events) {

				for (uint32_t i = 0; i < listeners.size(); ++i) {

					listeners[i]->func(*event);

				}

				if (event->shouldDelete) {

				}
			}
		}
	}

}