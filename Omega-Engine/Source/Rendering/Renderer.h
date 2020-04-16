#pragma once

#include "Rendering/RenderQueue.h"
#include "omega-engine/Renderer.h"
#include "utility/CString.h"

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace VulkanAPI
{
// forward declearions
class Swapchain;
class VkDriver;
class ProgramManager;
class CmdBuffer;
class CBufferManager;
} // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class OEScene;
class PostProcessInterface;
class OEEngine;
class RenderGraph;
class EngineConfig;
struct RGraphContext;;
struct RGraphPassContext;

class RenderStageBase
{
public:
    
    virtual ~RenderStageBase() = default;

    RenderStageBase(Util::String id) : passId(id)
    {
    }

    // ====== abstract functions ========
    virtual bool init(VulkanAPI::ProgramManager* manager) = 0;
    virtual void setupPass() = 0;

protected:
    Util::String passId;
};

class OERenderer : public Renderer
{

public:
    /**
     * @brief All of the stages supported by the renderer. All have an input and output which can be linked
     * to the next stage Stages can be removed except were stated. At the moment, only a deferred
     * staged renderer is supported but could easily add forward renderers, etc.
     */
    enum class RenderStage
    {
        IndirectLighting, //< Part of the global illumination pipeline
        GBufferFill,
        LightingPass,
        ForwardPass,
        Skybox,
        PreProcessPass,
        Composition,
        Count
    };

    /**
     * Each stage required for a deferred renderer. Some of these stages can be switched off.
     */
    const std::array<RenderStage, 5> deferredStages = {
        RenderStage::IndirectLighting,
        RenderStage::GBufferFill,
        RenderStage::LightingPass,
        RenderStage::Skybox,
        RenderStage::Composition
    };

    OERenderer(
        OEEngine& engine, OEScene& scene, VulkanAPI::Swapchain& swapchain, EngineConfig& config);
    ~OERenderer();

    /**
     @brief Creates all render stages needed for the rendering pipeline
     */
    bool prepare();

    void beginFrame();
    
    /**
     @brief calls the setup function for each render pass registered with the graph
     */
    void preparePasses();

    /**
     @brief Priimarily iterates over all visible renderable data within the scene and ceates the
     render queue.
     */
    bool update();

    /**
     @brief Draws into the cmd buffers all the data that is currently held by the scene
     */
    void draw();

    void drawQueueThreaded(VulkanAPI::CBufferManager& manager, RGraphContext& rgraphContext, RGraphPassContext& rpassContext);

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
    OEEngine& engine;
    OEScene& scene;

    EngineConfig& config;
};

} // namespace OmegaEngine
