#pragma once

#include "RenderGraph/RenderGraph.h"

#include "Rendering/IblImage.h"

#include <functional>
#include <vector>

namespace VulkanAPI
{
// forward declearions
class Swapchain;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class Scene;
class PostProcessInterface;
class Engine;

class Renderer
{

public:
	Renderer(Engine& engine, Scene& scene, VulkanAPI::Swapchain& swapchain);
	~Renderer();

	void init();

	// abstract override
	void render(std::unique_ptr<RenderQueue>& renderQueue) override;

	void createGbufferPass();
	void createDeferredPipeline(std::unique_ptr<VulkanAPI::BufferManager>& bufferManager,
	                            VulkanAPI::Swapchain& swapchain);
	void createDeferredPass();

	void renderDeferredPass(std::unique_ptr<VulkanAPI::CommandBuffer>& cmdBuffer);

private:
	VkContext context;

	// IBL environment mapping handler
	Ibl::IblImage ibl;

	// the post-processing manager
	PostProcessInterface postProcessInterface;

	// keep a local copy of the render config
	RenderConfig renderConfig;

	RenderGraph rGraph;

	// swap chain used for rendering to surface
	VulkanAPI::Swapchain& swapchain;
	
	// locally stored
	Engine& engine;
	Scene& scene;
	
};

}    // namespace OmegaEngine