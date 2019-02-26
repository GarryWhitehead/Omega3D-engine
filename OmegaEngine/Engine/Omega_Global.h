
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
		void init_eventManager();

		void init();

		// current state of the application
		class ProgramState
		{
		public:

			bool is_running() const
			{
				return isRunning;
			}

			float get_dt() const
			{
				return dt;
			}

			uint32_t get_win_width() const
			{
				return win_width;
			}

			uint32_t get_win_height() const
			{
				return win_height;
			}

			float get_mouse_sensitivity() const
			{
				return mouse_sensitivity;
			}

		private:

			// general
			bool isRunning = false;
			float dt = 30.0f;

			// mouse config
			float mouse_sensitivity = 0.25f;

			// window dimensions
			uint32_t win_width = 1200;
			uint32_t win_height = 800;
		};

		static ProgramState program_state;
	}
}




