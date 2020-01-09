#pragma once

#include "RenderGraph/Resources.h"

#include "Rendering/Renderer.h"

//forward declerations
namespace VulkanAPI
{
class ProgramManager;
}

namespace OmegaEngine
{
// forward declerations
class RenderGraph;
class OESkybox;

class SkyboxPass : public RenderStageBase
{

public:
    
	SkyboxPass(RenderGraph& rGraph, Util::String id, OESkybox& skybox);
	~SkyboxPass();

	// not copyable
	SkyboxPass(const SkyboxPass&) = delete;
	SkyboxPass& operator=(const SkyboxPass&) = delete;

	bool prepare(VulkanAPI::ProgramManager* manager) override;

private:

	// points to the render graph associated with this pass
	RenderGraph& rGraph;
    
    // keep refernce to the current skybox
    OESkybox& skybox;

	ResourceHandle output;
};

}    // namespace OmegaEngine
