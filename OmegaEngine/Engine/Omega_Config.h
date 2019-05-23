#pragma once
#include <cstdint>

namespace OmegaEngine
{

	struct EngineConfig
	{
		float fps = 30.0f;

		float mouseSensitivity = 0.1f;

		// desired screen dimensions
		uint32_t screenWidth = 1280;
		uint32_t screenHeight = 700;

		// states whether we want a dynamic or static scene
		// Mainly used to set whether the cmd buffers will be recordered each frame
		uint32_t sceneType = 0;
	};

}