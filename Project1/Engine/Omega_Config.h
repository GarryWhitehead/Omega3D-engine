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

	struct OmageaConfig
	{
		
		RenderConfig renderConfig;

		bool showUi = false;

		enum class AntiAlaisingMode
		{

		};
		AntiAlaisingMode aaMode;

		float targetFrameRate = 30.0f;
	};

}