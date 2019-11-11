#pragma once

#include "RenderGraph/Resources.h"

#include "Rendering/Renderer.h"

// forward declerations
namespace VulkanAPI
{
class ShaderManager;
class VkContext;
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

	LightingPass(RenderGraph& rGraph, VulkanAPI::ShaderManager& manager, Util::String id);

	// no copying
	LightingPass(const LightingPass&) = delete;
	LightingPass& operator=(const LightingPass&) = delete;

	bool create() override;
	bool preparePass(RGraphContext& context);

private:
	
	// points to the render graph associated with this pass
	RenderGraph& rGraph;

	// a pass must have a shader
	VulkanAPI::ShaderHandle handle;

	LPassInfo passInfo;
};
}    // namespace OmegaEngine
