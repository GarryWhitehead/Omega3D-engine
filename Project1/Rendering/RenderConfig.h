#pragma once

namespace OmegaEngine
{
	// forward declerations
	enum class RendererType;

	struct RenderConfig
	{
		struct General
		{
			RendererType renderer;

		} general;

		bool useSSAO = false;
		bool useMSAA = false;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool fogEnabled = true;
	};

}