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
class Skybox;

class SkyboxPass : public RenderStageBase
{

public:
    
	SkyboxPass(RenderGraph& rGraph, Util::String id, Skybox& skybox);
	~SkyboxPass();

	// not copyable
	SkyboxPass(const SkyboxPass&) = delete;
	SkyboxPass& operator=(const SkyboxPass&) = delete;

	bool prepare(VulkanAPI::ProgramManager* manager) override;

private:

	// points to the render graph associated with this pass
	RenderGraph& rGraph;
    
    // keep refernce to the current skybox
    Skybox& skybox;

	ResourceHandle output;
};

}    // namespace OmegaEngine
