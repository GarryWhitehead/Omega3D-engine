#pragma once

#include "RenderGraph/RenderGraphBuilder.h"

#include "Rendering/Renderer.h"

// forward declerations
namespace VulkanAPI
{
class VkDriver;
class CommandBufferManager;
class ProgramManager;
};

namespace OmegaEngine
{

// forward declerations
class RenderGraph;

class LightingPass : public RenderStageBase
{
public:

	inline static const Util::String lightingId = "lighting.glsl";
    
	struct LPassInfo
	{
		ResourceHandle output;
	};

	LightingPass(RenderGraph& rGraph, Util::String id);

	// no copying
	LightingPass(const LightingPass&) = delete;
	LightingPass& operator=(const LightingPass&) = delete;

    bool prepare(VulkanAPI::ProgramManager* manager) override;

private:
	
	// points to the render graph associated with this pass
	RenderGraph& rGraph;

	LPassInfo passInfo;
};
}    // namespace OmegaEngine
