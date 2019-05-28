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
			bool usePostProcess = false;
			bool useSkybox = true;
			bool sortRenderQueue = true;
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

		struct ToneMapSettings
		{
			float expBias = 1.0f;
			float gamma = 0.5;
		} toneMapSettings;

		struct PostProcess
		{
			bool useSSAO = false;
			bool useMSAA = false;
			bool useHdr = false;
			bool shadowsEnabled = true;
			bool bloomEnabled = true;
			bool fogEnabled = true;
		} postProcess;

		// shadows
		uint32_t shadowWidth = 2048;
		uint32_t shadowHeight = 2048;
		vk::Format shadowFormat = vk::Format::eD16Unorm;

		float biasConstant = 1.25f;
		float biasSlope = 1.75f;
		float biasClamp = 0.0f;
	};

}