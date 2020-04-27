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

#include "GBufferFillPass.h"

#include "Components/RenderableManager.h"
#include "Core/engine.h"
#include "ModelImporter/MaterialInstance.h"
#include "ModelImporter/MeshInstance.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "Rendering/RenderQueue.h"
#include "Scripting/OEConfig.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/ProgramManager.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"
#include "utility/Compiler.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

GBufferFillPass::GBufferFillPass(
    VulkanAPI::VkDriver& driver,
    RenderGraph& rGraph,
    Util::String id,
    OERenderableManager& rendManager,
    EngineConfig& config)
    : RenderStageBase(id)
    , driver(driver)
    , vkContext(driver.getContext())
    , rGraph(rGraph)
    , rendManager(rendManager)
    , config(config)
{
}

bool GBufferFillPass::init(VulkanAPI::ProgramManager* manager)
{
    OE_UNUSED(manager);
    depthFormat = VulkanAPI::VkUtil::getSupportedDepthFormat(vkContext.physical);
    return true;
}

void GBufferFillPass::setupPass()
{
    // shaders are prepared within the renderable manager for this pass
    // a list of the formats required for each buffer
    RenderGraphBuilder builder = rGraph.createPass(passId, RenderGraphPass::Type::Graphics);

    // create the gbuffer textures
    gbufferInfo.tex.position =
        builder.createRenderTarget("positionRT", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.colour =
        builder.createRenderTarget("baseColourRT", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.normal =
        builder.createRenderTarget("normalRT", 2048, 2048, vk::Format::eR8G8B8A8Unorm);
    gbufferInfo.tex.pbr =
        builder.createRenderTarget("pbrRT", 2048, 2048, vk::Format::eR16G16Sfloat);
    gbufferInfo.tex.emissive =
        builder.createRenderTarget("emissiveRT", 2048, 2048, vk::Format::eR16G16B16A16Sfloat);
    gbufferInfo.tex.depth = builder.createRenderTarget("depthRT", 2048, 2048, depthFormat);

    // create the output taragets
    gbufferInfo.attach.position = builder.addWriter("position", gbufferInfo.tex.position);
    gbufferInfo.attach.colour = builder.addWriter("colour", gbufferInfo.tex.colour);
    gbufferInfo.attach.normal = builder.addWriter("normal", gbufferInfo.tex.normal);
    gbufferInfo.attach.pbr = builder.addWriter("pbr", gbufferInfo.tex.pbr);
    gbufferInfo.attach.emissive = builder.addWriter("emissive", gbufferInfo.tex.emissive);
    gbufferInfo.attach.depth = builder.addWriter("depth", gbufferInfo.tex.depth);

    OEMaths::colour4 clear = config.findOrInsertVec4("clearValue", OEEngine::Default_ClearVal);
    builder.setClearColour(clear);
    builder.setDepthClear(1.0f);

    builder.addExecute([=](RGraphPassContext& rpassContext, RGraphContext& rgraphContext) {
        // draw the contents of the renderable rendder queue
        OERenderer* renderer = rgraphContext.renderer;
        renderer->drawQueueThreaded(driver.getCbManager(), rgraphContext, rpassContext);
    });
}

void GBufferFillPass::drawCallback(
    VulkanAPI::CmdBuffer* cmdBuffer,
    void* data,
    RGraphContext& rgraphContext,
    RGraphPassContext& rpassContext)
{
    Renderable* render = static_cast<Renderable*>(data);
    VulkanAPI::ShaderProgram* prog = render->program;
    Material* mat = render->material;

    assert(render && mat && prog);

    auto& cbManager = rgraphContext.driver->getCbManager();

    // merge the material set with the mesh ubo sets
    std::vector<VulkanAPI::DescriptorSetInfo> uboSetInfo =
        cbManager.findDescriptorSets(prog->getShaderId());
    assert(!uboSetInfo.empty());

    std::vector<vk::DescriptorSet> descrSets(uboSetInfo.size() + 1);
    for (uint64_t i = 0; i < uboSetInfo.size(); ++i)
    {
        descrSets[i] = uboSetInfo[i].descrSet;
    }
    descrSets[descrSets.size() - 1] = *mat->descriptorSet;

    // ==================== bindings ==========================

    assert(render->meshDynamicOffset != UINT32_MAX);
    std::vector<uint32_t> offsets {render->meshDynamicOffset};
    if (render->skinDynamicOffset != UINT32_MAX)
    {
        offsets.emplace_back(render->skinDynamicOffset);
    }

    cmdBuffer->bindPipeline(
        cbManager, rpassContext.rpass, rpassContext.fbo, prog, VulkanAPI::Pipeline::Type::Graphics);

    cmdBuffer->bindDescriptors(
        prog->getPLineLayout()->get(), descrSets, offsets, VulkanAPI::Pipeline::Type::Graphics);

    // the push block contains all the material attributes for this mesh
    cmdBuffer->bindPushBlock(
        prog,
        vk::ShaderStageFlagBits::eFragment,
        sizeof(MaterialInstance::MaterialBlock),
        &render->material->instance->block);

    // TODO: check the offsets here - we aren't setting them at present
    cmdBuffer->bindVertexBuffer(render->vertBuffer->get(), 0);
    cmdBuffer->bindIndexBuffer(render->indexBuffer->get(), 0);

    // draw all the primitives
    for (const MeshInstance::Primitive& prim : render->instance->primitives)
    {
        cmdBuffer->drawIndexed(prim.indexCount, prim.indexPrimitiveOffset);
    }
}


} // namespace OmegaEngine
