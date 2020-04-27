/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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
struct RGraphContext;
;
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
     * @brief All of the stages supported by the renderer. All have an input and output which can be
     * linked to the next stage Stages can be removed except were stated. At the moment, only a
     * deferred staged renderer is supported but could easily add forward renderers, etc.
     */
    enum class RenderStage
    {
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
    const std::array<RenderStage, 4> deferredStages = {RenderStage::GBufferFill,
                                                       RenderStage::LightingPass,
                                                       RenderStage::Skybox,
                                                       RenderStage::Composition};

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
     @brief Draws into the cmd buffers all the data that is currently held by the scene
     */
    bool draw();

    void drawQueueThreaded(
        VulkanAPI::CBufferManager& manager,
        RGraphContext& rgraphContext,
        RGraphPassContext& rpassContext);

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
