
#include "Omega_Global.h"

#include "Utility/file_log.h"
#include "Managers/FileManager.h"
#include "Managers/EventManager.h"
#include "VulkanCore/VkRenderManager.h"

#include <assert.h>

namespace OmegaEngine
{
	namespace Global
	{
		void init_logger()
		{
			fileLog = new FileLog;
			assert(fileLog != nullptr);
		}

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

		void init()
		{
			init_logger();
			init_eventManager();
			init_vkRenderer();
		}
	}
}