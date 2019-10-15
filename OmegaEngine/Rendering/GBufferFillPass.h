#pragma once

#include "RenderGraph/RenderGraph.h"

namespace OmegaEngine
{

class GBufferFillPass
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

	void init();

private:
	RenderGraph* rGraph = nullptr;

	GBufferInfo gbufferInfo;
};

}    // namespace OmegaEngine
