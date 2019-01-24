
#include "Omega_Global.h"

#include "Managers/EventManager.h"

#include <assert.h>

namespace OmegaEngine
{
	namespace Global
	{
		void init_eventManager()
		{
			managers.eventManager = new EventManager;
			assert(managers.eventManager != nullptr);
		}

		void init()
		{
			init_eventManager();
		}
	}
}