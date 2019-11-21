#pragma once

#include "Rendering/Renderer.h"

namespace OmegaEngine
{

class CompositionPass : public RenderStageBase
{
public:

	CompositionPass(RenderGraph& rGraph, Util::String id);
	~CompositionPass();

	bool prepare(VulkanAPI::ShaderManager* manager) override;

private:

	// points to the render graph associated with this pass
	RenderGraph& rGraph;

};

}