
#include "Omega_Global.h"

#include "Managers/FileManager.h"
#include "Managers/EventManager.h"
#include "VulkanCore/VkRenderManager.h"

#include <assert.h>

namespace OmegaEngine
{
	namespace Global
	{
		void init_fileManager()
		{
			managers.fileManager = new FileManager;
			assert(managers.fileManager != nullptr);
		}

		void init_eventManager()
		{
			managers.eventManger = new EventManager;
			assert(managers.eventManger != nullptr);
		}

		void init_vkRenderer()
		{
			managers.renderManager = new VkRenderManager();
			assert(managers.renderManager != nullptr);
		}

		void init()
		{
			init_fileManager();
			init_eventManager();
			init_vkRenderer();
		}
	}
}