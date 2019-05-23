#pragma once

#include "Vulkan/Common.h"
#include "rapidjson/document.h"

#include <array>

namespace OmegaEngine
{
	// forward declerations
	enum class RendererType;

	struct RenderConfig
	{
		// fills the config struct with data if available
		void load();

		struct General
		{
			// type of renderer to use - at the moment only deferred is supported
			RendererType renderer;

			std::array<float, 4> backgroundColour = {0.0f, 0.0f, 0.0f, 1.0f};
			bool use_post_process = false;
			bool useSkybox = true;
			bool sort_renderQueue = true;
			bool useStockModels = true;
			
		} general;

		struct Deferred
		{
			uint32_t gBufferWidth = 2048;
			uint32_t gBufferHeight = 2048;
			uint32_t offscreenWidth = 2048;
			uint32_t offscreenHeight = 2048;
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

		// shadows
		uint32_t shadow_width = 2048;
		uint32_t shadow_height = 2048;
		vk::Format shadow_format = vk::Format::eD16Unorm;

		float biasConstant = 1.25f;
		float biasSlope = 1.75f;
		float biasClamp = 0.0f;
	};

}