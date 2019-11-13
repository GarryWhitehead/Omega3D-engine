#pragma once

#include "RenderGraph/RenderGraph.h"

#include "Rendering/IblImage.h"
#include "Rendering/RenderQueue.h"

#include <array>
#include <functional>
#include <vector>

namespace VulkanAPI
{
// forward declearions
class Swapchain;
class VkContext;
class ShaderManager;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class Scene;
class PostProcessInterface;
class Engine;

class RenderStageBase
{
public:
	RenderStageBase(Util::String id)
	{
		passId = id;
	}

	// ====== abstract functions ========
	virtual bool prepare(VulkanAPI::ShaderManager* manager) = 0;

protected:
	static Util::String passId;
};

/**
* The current render context. This is primarily used for passing to the each render pass
*/
struct RenderContext
{
	// the render queue - passed from the scene to the renderer after visibility checks (TODO: add visibility checks!!)
	RenderQueue renderQueue;

};

class Renderer
{

public:
	/**
     * All of the stages supported by the renderer. All have an input and output which can be linked to the next stage
     * Stages can be removed except were stated. At the moment, only a deferred staged renderer is supported but
     * could easily add forward renderers, etc.
     */
	enum class RenderStage
	{
		GBufferFill,
		LightingPass,
		ForwardPass,
		PreProcessPass,
		Count
	};

	/**
     * Each stage required for a deferred renderer. Some of these stages can be switched off.
     */
	const std::array<RenderStage, 2> deferredStages = { RenderStage::GBufferFill, RenderStage::LightingPass };

	Renderer(Engine& engine, Scene& scene, VulkanAPI::Swapchain& swapchain);
	~Renderer();

	/**
     * Creates all render stages needed for the rendering pipeline
     */
	void prepare();

	// abstract override
	void render();


private:
	// The current vulkan instance
	VulkanAPI::VkContext& context;

	// IBL environment mapping handler
	Ibl::IblImage ibl;

	// current render context
	RenderContext rContext;

	// the post-processing manager
	PostProcessInterface postProcessInterface;

	// keep a local copy of the render config
	RenderConfig renderConfig;

	// Each rendering stage of this renderer
	std::vector<std::unique_ptr<RenderStageBase>> rStages;

	RenderGraph rGraph;

	// swap chain used for rendering to surface
	VulkanAPI::Swapchain& swapchain;

	// locally stored
	Engine& engine;
	Scene& scene;
};

}    // namespace OmegaEngine
