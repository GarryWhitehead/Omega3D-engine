
/// Contains all of the global variables that will be used for the Omega Engine 
///
/// 

#pragma once

// forward decleartions
class VkRenderManager;
class FileLog;

namespace OmegaEngine
{
	class EventManager;
	class ObjectManager;

	namespace Global
	{
		
		struct Managers
		{
			EventManager* eventManager;
			VkRenderManager* renderManager;
			ObjectManager* objectManager;
		};

		static Managers managers;

		// all global initilisation functions for global managers
		void init_eventManager();
		void init_vkRenderer();
		void init_objectManager();

		void init();

		// for file logging purposes
		FileLog* fileLog;

		// current state of the application
		struct ProgramState
		{
			bool isRunning = false;
			float dt;
		};

		static ProgramState programState;
	}
}




