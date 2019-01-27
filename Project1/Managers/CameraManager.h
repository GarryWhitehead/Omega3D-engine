#pragma once
#include "ComponentInterface/ComponentManagerBase.h"

namespace OmegaEngine 
{

	class CameraManager : public ComponentManagerBase
	{

	public:

		CameraManager();
		~CameraManager();

		// event functions
		void keyboard_press_event(KeyboardPressEvent& event);
		void mouse_button_event(MouseButtonEvent& event);

	private:

	};

}

