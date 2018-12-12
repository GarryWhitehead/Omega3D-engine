
/// Contains all of the global variables that will be used for the Omega Engine 
///
/// 

#pragma once

// forward decleartions
class MessageHandler;
class RenderManager;

namespace OmegaEngine
{
	namespace Global
	{
		struct Managers
		{
			MessageHandler* messageHandler;
			VkRenderManager* renderManager;
		};

		static Managers managers;

		// all global initilisation functions for global managers
		void init_messageHandler();

		void init_vkRenderer();

		void init();
	}
}


