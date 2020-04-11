#pragma once

#include "RenderGraph/RenderGraphBuilder.h"

#include "Rendering/Renderer.h"

// forward declerations
namespace VulkanAPI
{
class VkDriver;
class CommandBufferManager;
class ProgramManager;
class ShaderProgram;
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
    
    bool init(VulkanAPI::ProgramManager* manager) override;
    void setupPass() override;

private:
	
	// points to the render graph associated with this pass
	RenderGraph& rGraph;

	LPassInfo passInfo;
    
    VulkanAPI::ShaderProgram* prog = nullptr;
};
}    // namespace OmegaEngine
