#pragma once
#include <cstdint>

namespace OmegaEngine
{

	struct EngineConfig
	{
		float fps = 30.0f;

		float mouse_sensitivity = 0.1f;

		// desired screen dimensions
		uint32_t screen_width = 1280;
		uint32_t screen_height = 700;

		// states whether we want a dynamic or static scene
		// Mainly used to set whether the cmd buffers will be recordered each frame
		uint32_t scene_type = 0;
	};

}