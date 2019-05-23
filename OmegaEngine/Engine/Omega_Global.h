
/// Contains all of the global variables that will be used for the Omega Engine 
///
/// 

#pragma once

#include <cstdint>

namespace OmegaEngine
{
	class EventManager;

	namespace Global
	{

		EventManager* eventManager();

		// all global initilisation functions for global managers
		void initEventManager();

		void init();

		
	}
}




