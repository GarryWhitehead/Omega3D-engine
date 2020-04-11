#pragma once

#include "RenderGraph/RenderGraphBuilder.h"
#include "Rendering/Renderer.h"

#include "VulkanAPI/Shader.h"

// forward decleartions
namespace VulkanAPI
{
class ProgramManager;
class CmdBuffer;
class VkDriver;
struct VkContext;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class Renderer;
class OERenderableManager;
class EngineConfig;

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

	GBufferFillPass(VulkanAPI::VkDriver& vkDriver, RenderGraph& rGraph, Util::String id, OERenderableManager& rendManager, EngineConfig& config);

	// no copying
	GBufferFillPass(const GBufferFillPass&) = delete;
	GBufferFillPass& operator=(const GBufferFillPass&) = delete;
    
    bool init(VulkanAPI::ProgramManager* manager) override;
    
	/// creates a new renderpass with the required inputs/outputs and prepares the render func
	void setupPass() override;

	friend class Renderer;

	// draw callback function used in the render queue
	static void drawCallback(VulkanAPI::CmdBuffer* cmdBuffer, void* instance, RGraphContext& context);

private:
    
    VulkanAPI::VkDriver& driver;
    VulkanAPI::VkContext& vkContext;
    
	// reference to the render graph associated with this pass
	RenderGraph& rGraph;

	// the gbuffer uses data from the renderable manager, namely materials.
	// other vertex data will be from the render queue, considering visibility
	OERenderableManager& rendManager;
    EngineConfig& config;
    
	GBufferInfo gbufferInfo;
    vk::Format depthFormat;
};

}    // namespace OmegaEngine
