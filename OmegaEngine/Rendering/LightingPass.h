#pragma once

#include "RenderGraph/Resources.h"

#include "Rendering/Renderer.h"

// forward declerations
namespace VulkanAPI
{
class ShaderManager;
};

namespace OmegaEngine
{

// forward declerations
class RenderGraph;

class LightingPass : public RenderStageBase
{
public:

	struct LPassInfo
	{
		ResourceHandle output;
	};

	LightingPass()
	{
	}

	// no copying
	LightingPass(const LightingPass&) = delete;
	LightingPass& operator=(const LightingPass&) = delete;

	bool prepare(RenderGraph& rGraph, VulkanAPI::ShaderManager* manager) override;

private:

	RenderGraph* rGraph = nullptr;

	LPassInfo passInfo;
};
}    // namespace OmegaEngine
