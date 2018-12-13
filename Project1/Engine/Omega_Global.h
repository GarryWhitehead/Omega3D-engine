
/// Contains all of the global variables that will be used for the Omega Engine 
///
/// 

#pragma once

// forward decleartions
class MessageHandler;
class RenderManager;
class FileManager;

namespace OmegaEngine
{
	namespace Global
	{
		struct Managers
		{
			FileManager* fileManager;
			MessageHandler* messageHandler;
			VkRenderManager* renderManager;
		};

		static Managers managers;

		// all global initilisation functions for global managers
		void init_fileManager();
		void init_messageHandler();

		void init_vkRenderer();

		void init();
	}
}


