#pragma once

#include <array>

namespace OmegaEngine
{
	// forward declerations
	enum class RendererType;

	struct RenderConfig
	{
		struct General
		{
			// type of renderer to use - at the moment only deferred is supported
			RendererType renderer;

			std::array<float, 4> background_col = {0.0f, 0.0f, 0.0f, 1.0f};
			bool use_post_process = false;

		} general;

		struct Deferred
		{
			uint32_t gbuffer_width = 2048;
			uint32_t gbuffer_height = 2048;
			uint32_t offscreen_width = 2048;
			uint32_t offscreen_height = 2048;
		} deferred;

		struct IBLInfo
		{
			float ambientScale = 1.0f;
			bool isContributing = false;
		} ibl;

		bool useSSAO = false;
		bool useMSAA = false;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool fogEnabled = true;
	};

}