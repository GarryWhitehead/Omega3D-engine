#pragma once

#include "Rendering/Renderer.h"
#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/Shader.h"

// forward decleartions
namespace VulkanAPI
{
class ShaderManager;
class CmdBuffer;
}

namespace OmegaEngine
{
//forward declerations
class Renderer;
class RenderableManager;

class GBufferFillPass : public RenderStageBase
{
public:
	struct GBufferInfo
	{
		struct Textures
		{
			ResourceHandle position;
			ResourceHandle colour;
			ResourceHandle normal;
			ResourceHandle emissive;
			ResourceHandle pbr;
			ResourceHandle depth;
		} tex;

		struct Atachments
		{
			AttachmentHandle position;
			AttachmentHandle colour;
			AttachmentHandle normal;
			AttachmentHandle emissive;
			AttachmentHandle pbr;
			AttachmentHandle depth;
		} attach;
	};

	GBufferFillPass(RenderGraph& rGraph, Util::String id, RenderableManager& rendManager);

	// no copying
	GBufferFillPass(const GBufferFillPass&) = delete;
	GBufferFillPass& operator=(const GBufferFillPass&) = delete;

	/**
	* @brief This should be called if the materials list held by the renderable manager has been updated.
	*/
	bool update(VulkanAPI::ShaderManager* manager);

    bool prepare(VulkanAPI::ShaderManager* manager) override;

	// draw callback function used in the render queue
	void drawCallback(VulkanAPI::CmdBuffer* cmdBuffer,void* instance);

private:
    
    Renderer* renderer = nullptr;
    
	// reference to the render graph associated with this pass
	RenderGraph& rGraph;

	// the gbuffer uses data from the renderable manager, namely materials.
	// other vertex data will be from the render queue, considering visibility
	RenderableManager& rendManager;

	GBufferInfo gbufferInfo;
};

}    // namespace OmegaEngine
