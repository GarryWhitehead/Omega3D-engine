
#include "Omega_Global.h"

#include "Utility/FileManager.h"
#include "Utility/message_handler.h"
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

		void init_messageHandler()
		{
			managers.messageHandler = new MessageHandler;
			assert(managers.messageHandler != nullptr);
		}

		void init_vkRenderer()
		{
			managers.renderManager = new VkRenderManager();
			assert(managers.renderManager != nullptr);
		}

		void init()
		{
			init_fileManager();
			init_messageHandler();
			init_vkRenderer();
		}
	}
}