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

#include "RenderGraph/RenderGraphBuilder.h"
#include "Rendering/Renderer.h"

// forward declerations
namespace VulkanAPI
{
class ProgramManager;
class ShaderProgram;
} // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class RenderGraph;
class OEScene;

class SkyboxPass : public RenderStageBase
{

public:
    SkyboxPass(RenderGraph& rGraph, Util::String id, OEScene& scene);
    ~SkyboxPass();

    // not copyable
    SkyboxPass(const SkyboxPass&) = delete;
    SkyboxPass& operator=(const SkyboxPass&) = delete;

    bool init(VulkanAPI::ProgramManager* manager) override;
    void setupPass() override;

private:
    // points to the render graph associated with this pass
    RenderGraph& rGraph;

    // keep refernce to the current skybox
    OEScene& scene;

    ResourceHandle offscreenTex;
    VulkanAPI::ShaderProgram* prog = nullptr;
};

} // namespace OmegaEngine
