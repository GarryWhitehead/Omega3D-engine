#pragma once

#include "Rendering/Renderers/RendererBase.h"
#include "VulkanAPI/Common.h"
#include "rapidjson/document.h"

#include <array>

namespace OmegaEngine
{

struct RenderConfig
{
	// fills the config struct with data from file if available
	void load();

	struct General
	{
		// type of renderer to use - at the moment only deferred is supported
		RendererType renderer = RendererType::Deferred;

		std::array<float, 4> backgroundColour = { 0.0f, 0.0f, 0.0f, 1.0f };
		bool usePostProcess = false;
		bool useSkybox = true;
		bool sortRenderQueue = true;
		bool useStockModels = true;
		bool hasIblImages = false;

	} general;

	struct Deferred
	{
		uint32_t gBufferWidth = 2048;
		uint32_t gBufferHeight = 2048;
		uint32_t deferredWidth = 2048;
		uint32_t deferredHeight = 2048;
		vk::Format deferredFormat = vk::Format::eR16G16B16A16Sfloat;
	} deferred;

	struct IBLInfo
	{
		float ambientScale = 1.0f;
		bool isContributing = true;
	} ibl;

	struct ToneMapSettings
	{
		float expBias = 1.0f;
		float gamma = 2.2f;
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

} // namespace OmegaEngine