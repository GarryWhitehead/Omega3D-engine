#pragma once

namespace OmegaEngine
{

	struct RenderConfig
	{
		bool useSSAO = false;
		bool useMSAA = false;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool fogEnabled = true;
	};

	struct EngineConfig
	{
		float fps = 30.0f;
	};

}