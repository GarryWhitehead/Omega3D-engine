#pragma once

#include "Rendering/Renderer.h"
#include "RenderGraph/RenderGraph.h"

#include "VulkanAPI/Shader.h"

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

	GBufferFillPass(RenderGraph& rGraph, VulkanAPI::ShaderManager& manager, Util::String id);

	// no copying
	GBufferFillPass(const GBufferFillPass&) = delete;
	GBufferFillPass& operator=(const GBufferFillPass&) = delete;

	bool create() override;

	bool preparePass(RGraphContext& context);

private:

	// reference to the render graph associated with this pass
	RenderGraph& rGraph;

	// and the shader manager as this is contains a large element of the render data
	VulkanAPI::ShaderManager& shaderMan;

	// a pass must have a shader
	VulkanAPI::ShaderHandle handle;

	GBufferInfo gbufferInfo;
};

}    // namespace OmegaEngine
