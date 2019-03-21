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

			std::array<float, 4> background_col = {};
			bool use_post_process = false;

		} general;

		struct Deferred
		{
			uint32_t gbuffer_width = 1024;
			uint32_t gbuffer_height = 1024;
			uint32_t offscreen_width = 1024;
			uint32_t offscreen_height = 1024;
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