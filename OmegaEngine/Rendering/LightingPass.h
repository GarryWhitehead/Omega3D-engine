#pragma once

#include "RenderGraph/Resources.h"

#include "Rendering/Renderer.h"

// forward declerations
namespace VulkanAPI
{
class VkContext;
class CmdBufferManager;
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

	LightingPass(RenderGraph& rGraph, Util::String id);

	// no copying
	LightingPass(const LightingPass&) = delete;
	LightingPass& operator=(const LightingPass&) = delete;

    bool prepare(VulkanAPI::ShaderManager* manager) override;

private:
	
	// points to the render graph associated with this pass
	RenderGraph& rGraph;
    
    VulkanAPI::CmdBufferManager& cbManager;

	LPassInfo passInfo;
};
}    // namespace OmegaEngine
