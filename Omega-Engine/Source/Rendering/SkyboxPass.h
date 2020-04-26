#pragma once

#include "RenderGraph/RenderGraphBuilder.h"

#include "Rendering/Renderer.h"

//forward declerations
namespace VulkanAPI
{
class ProgramManager;
class ShaderProgram;
}

namespace OmegaEngine
{
// forward declerations
class RenderGraph;
class OEScene;

class SkyboxPass : public RenderStageBase
{

public:
    
	SkyboxPass(RenderGraph& rGraph, Util::String id, OEScene& scene);
	~SkyboxPass();

	// not copyable
	SkyboxPass(const SkyboxPass&) = delete;
	SkyboxPass& operator=(const SkyboxPass&) = delete;
    
    bool init(VulkanAPI::ProgramManager* manager) override;
	void setupPass() override;

private:

	// points to the render graph associated with this pass
	RenderGraph& rGraph;
    
    // keep refernce to the current skybox
    OEScene& scene;

	ResourceHandle offscreenTex;
    VulkanAPI::ShaderProgram* prog = nullptr;
};

}    // namespace OmegaEngine
