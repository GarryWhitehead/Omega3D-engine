#pragma once

#include "Rendering/Renderer.h"
#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/Shader.h"

// forward decleartions
namespace VulkanAPI
{
class ShaderManager;
}

namespace OmegaEngine
{

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

	GBufferFillPass(RenderGraph& rGraph, Util::String id);

	// no copying
	GBufferFillPass(const GBufferFillPass&) = delete;
	GBufferFillPass& operator=(const GBufferFillPass&) = delete;

    bool prepare(VulkanAPI::ShaderManager* manager) override;

	// draw callback function used in the render queue
	void drawCallback();

private:

	// reference to the render graph associated with this pass
	RenderGraph& rGraph;

	GBufferInfo gbufferInfo;
};

}    // namespace OmegaEngine
