
#include "Omega_Global.h"

#include "Utility/message_handler.h"

#include <assert.h>

namespace OmegaEngine
{
	namespace Global
	{

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
			init_messageHandler();
			init_renderManager();
		}
	}
}