#pragma once

#include "Rendering/RenderQueue.h"

#include "utility/CString.h"

#include <array>
#include <functional>
#include <vector>
#include <memory>

namespace VulkanAPI
{
// forward declearions
class Swapchain;
class VkDriver;
class ProgramManager;
class CmdBuffer;
class CmdBufferManager;
}    // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class Scene;
class PostProcessInterface;
class Engine;
class RenderGraph;
class EngineConfig;

class RenderStageBase
{
public:
    
    virtual ~RenderStageBase() = default;
    
	RenderStageBase(Util::String id)
	{
		passId = id;
	}

	// ====== abstract functions ========
	virtual bool prepare(VulkanAPI::ProgramManager* manager) = 0;

protected:
    
	static Util::String passId;
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
		IndirectLighting,      //< Part of the global illumination pipeline
        GBufferFill,
		LightingPass,
		ForwardPass,
		Skybox,
		PreProcessPass,
		Count
	};

	/**
     * Each stage required for a deferred renderer. Some of these stages can be switched off.
     */
	const std::array<RenderStage, 3> deferredStages = { RenderStage::GBufferFill, RenderStage::LightingPass,
		                                                RenderStage::Skybox };

	Renderer(Engine& engine, Scene& scene, VulkanAPI::Swapchain& swapchain, EngineConfig& config);
	~Renderer();
    
	/**
     * @brief Creates all render stages needed for the rendering pipeline
     */
	void prepare();
    
    void beginFrame();
    
    /**
     * @brief Priimarily iterates over all visible renderable data within the scene and ceates the render queue.
     */
    void update();
    
	/**
	* @brief Draws into the cmd buffers all the data that is currently held by the scene
	*/
	void draw();
    
    void drawQueueThreaded(VulkanAPI::CmdBuffer& cmdBuffer, VulkanAPI::CmdBufferManager& manager, RGraphContext& context);
    
	using RenderStagePtr = std::unique_ptr<RenderStageBase>;

private:

	/// The current vulkan instance
	VulkanAPI::VkDriver& vkDriver;

	/// Each rendering stage of this renderer
	std::vector<RenderStagePtr> rStages;

	/// Contains the layout of the rendering stages
	std::unique_ptr<RenderGraph> rGraph;

	// swap chain used for rendering to surface
	VulkanAPI::Swapchain& swapchain;

	// locally stored
	Engine& engine;
	Scene& scene;
    
    EngineConfig& config;
};

}    // namespace OmegaEngine
