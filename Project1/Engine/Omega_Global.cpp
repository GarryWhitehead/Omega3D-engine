
#include "Omega_Global.h"

#include "Managers/EventManager.h"
#include "Vulkan/VkRenderManager.h"
#include "Managers/ObjectManager.h"

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

		void init_vkRenderer()
		{
			managers.renderManager = new VkRenderManager();
			assert(managers.renderManager != nullptr);
		}

		void init_objectManager()
		{
			managers.objectManager = new ObjectManager;
			assert(managers.objectManager != nullptr);
		}

		void init()
		{
			init_eventManager();
			init_vkRenderer();
			init_objectManager();
		}
	}
}