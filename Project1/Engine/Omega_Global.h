
/// Contains all of the global variables that will be used for the Omega Engine 
///
/// 

#pragma once

// forward decleartions

class VkRenderManager;
class FileManager;

namespace OmegaEngine
{
	class EventManager;

	namespace Global
	{
		
		struct Managers
		{
			FileManager* fileManager;
			EventManager* eventManger;
			VkRenderManager* renderManager;
		};

		static Managers managers;

		// all global initilisation functions for global managers
		void init_fileManager();
		void init_eventManager();
		void init_vkRenderer();

		void init();
	}
}


