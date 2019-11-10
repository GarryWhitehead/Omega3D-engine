#pragma once

#include "Rendering/Renderers/DeferredRenderer.h"
#include "RenderGraph/RenderGraph.h"

namespace VulkanAPI
{
    class ShaderManager;
};

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

	GBufferFillPass()
	{
	}

	// no copying
	GBufferFillPass(const GBufferFillPass&) = delete;
	GBufferFillPass& operator=(const GBufferFillPass&) = delete;

	bool prepare(RenderGraph& rGraph, VulkanAPI::ShaderManager* manager) override;

private:

	GBufferInfo gbufferInfo;
};

}    // namespace OmegaEngine
