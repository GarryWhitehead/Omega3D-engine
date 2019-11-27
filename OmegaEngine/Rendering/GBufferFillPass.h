#pragma once

#include "RenderGraph/RenderGraph.h"
#include "Rendering/Renderer.h"

#include "VulkanAPI/Shader.h"

// forward decleartions
namespace VulkanAPI
{
class ShaderManager;
class CmdBuffer;
}    // namespace VulkanAPI

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

	/// creates a new renderpass with the required inputs/outputs and prepares the render func
	bool prepare(VulkanAPI::ShaderManager* manager) override;

private:

	// draw callback function used in the render queue
	void drawCallback(VulkanAPI::CmdBuffer& cmdBuffer, void* instance, RGraphContext& context);

private:

	// reference to the render graph associated with this pass
	RenderGraph& rGraph;

	// the gbuffer uses data from the renderable manager, namely materials.
	// other vertex data will be from the render queue, considering visibility
	RenderableManager& rendManager;

	GBufferInfo gbufferInfo;
};

}    // namespace OmegaEngine
