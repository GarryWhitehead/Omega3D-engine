#pragma once
#include <cstdint>

namespace OmegaEngine
{

	struct EngineConfig
	{
		float fps = 30.0f;

		float mouse_sensitivity = 0.25f;

		// desired screen dimensions
		uint32_t screen_width = 1280;
		uint32_t screen_height = 700;

	};

}