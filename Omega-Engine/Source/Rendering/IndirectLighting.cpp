#include "IndirectLighting.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"
#include "RenderGraph/RenderGraph.h"
#include "RenderGraph/RenderGraphPass.h"
#include "Rendering/SkyboxPass.h"
#include "Types/Skybox.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace OmegaEngine
{

IndirectLighting::IndirectLighting(RenderGraph& rGraph, Util::String id, OESkybox& skybox)
    : RenderStageBase(id), rGraph(rGraph), skybox(skybox)
{
}

IndirectLighting::~IndirectLighting()
{
}

void IndirectLighting::calculateCubeTransform(
    const uint32_t face,
    const float zNear,
    const float zFar,
    OEMaths::mat4f& outputProj,
    OEMaths::mat4f& outputView)
{
    OEMaths::vec3f target[6] = {
        OEMaths::vec3f(1.0f, 0.0f, 0.0f),
        OEMaths::vec3f(-1.0f, 0.0f, 0.0f),
        OEMaths::vec3f(0.0f, 1.0f, 0.0f),
        OEMaths::vec3f(0.0f, -1.0f, 0.0f),
        OEMaths::vec3f(0.0f, 0.0f, 1.0f),
        OEMaths::vec3f(0.0f, 0.0f, -1.0f)};

    OEMaths::vec3f cameraUp[6] = {
        OEMaths::vec3f(0.0f, 1.0f, 0.0f),
        OEMaths::vec3f(0.0f, 1.0f, 0.0f),
        OEMaths::vec3f(0.0f, 0.0f, -1.0f),
        OEMaths::vec3f(0.0f, 0.0f, 1.0f),
        OEMaths::vec3f(0.0f, 1.0f, 0.0f),
        OEMaths::vec3f(0.0f, 1.0f, 0.0f)};

    outputProj = outputProj.scale(OEMaths::vec3f {-1.0f, 1.0f, 1.0f}) *
        OEMaths::perspective(90.0f, 1.0f, zNear, zFar);
    OEMaths::vec3f pos {0.0f};
    outputView = OEMaths::lookAt(pos, target[face], cameraUp[face]);
}

bool IndirectLighting::init(VulkanAPI::ProgramManager* manager)
{
    // bdrf shader
    const Util::String bdrf_filename = "bdrf.glsl";
    bdrf_prog = manager->getVariantOrCreate(bdrf_filename, 0);
    if (!bdrf_prog)
    {
        return false;
    }
    
    // irradiance shader
    const Util::String irradiance_filename = "irradianceMap.glsl";
    irradiance_prog = manager->getVariantOrCreate(irradiance_filename, 0);
    if (!irradiance_prog)
    {
        return false;
    }
    
    // specular shader
    const Util::String specular_filename = "specularMap.glsl";
    specular_prog = manager->getVariantOrCreate(specular_filename, 0);
    if (!specular_prog)
    {
        return false;
    }
    return true;
}

void IndirectLighting::setupPass()
{
    const uint32_t lutDimensions = 512;
    vk::ClearColorValue clearValue;

    // bdrf
    {
        RenderGraphBuilder builder =
            rGraph.createPass("BdrfGenerationPass", RenderGraphPass::Type::Graphics);
        builder.setRenderPassFlag(RenderPassFlags::IntermitentPass);
        
        bdrfInfo.texture = builder.createRenderTarget(
            "bdrf_target", lutDimensions, lutDimensions, vk::Format::eR16G16Sfloat);

        bdrfInfo.attachment = builder.addWriter("BdrfSampler", bdrfInfo.texture);

        builder.addExecute([=](RGraphContext& context) {
            auto& cbManager = context.driver->getCbManager();
            VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getCmdBuffer();

            VulkanAPI::RenderPass* renderpass = rGraph.getRenderpass(context.rpass);
            cmdBuffer->bindPipeline(cbManager, renderpass, bdrf_prog);
            cmdBuffer->drawQuad();
        });
    }

    // irradiance
    {
        RenderGraphBuilder builder =
            rGraph.createPass("IrradiancePass", RenderGraphPass::Type::Graphics);
        builder.setRenderPassFlag(RenderPassFlags::IntermitentPass);
        
        irrInfo.texture = builder.createRenderTarget(
            "irradiance_target",
            irradianceMapDim,
            irradianceMapDim,
            vk::Format::eR32G32B32A32Sfloat);

        irrInfo.attachment = builder.addWriter("IrradianceSampler", irrInfo.texture);

        builder.addExecute([=](RGraphContext& context) {
            buildMap(context, irradiance_prog, irradianceMapDim, MapType::Irradiance, skybox);
        });
    }

    // specular
    {
        RenderGraphBuilder builder =
            rGraph.createPass("SpecularPass", RenderGraphPass::Type::Graphics);
        builder.setRenderPassFlag(RenderPassFlags::IntermitentPass);
        
        specInfo.texture = builder.createRenderTarget(
            "specular_target", specularMapDim, specularMapDim, vk::Format::eR32G32B32A32Sfloat);

        specInfo.attachment = builder.addWriter("SpecularSampler", specInfo.texture);

        builder.addExecute([=](RGraphContext& context) {
            buildMap(context, specular_prog, specularMapDim, MapType::Specular, skybox);
        });
    }
}

void IndirectLighting::buildMap(
    RGraphContext& context,
    VulkanAPI::ShaderProgram* prog,
    uint32_t dim,
    const MapType type,
    OESkybox& skybox)
{
    auto& cbManager = context.driver->getCbManager();
    VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getCmdBuffer();

    vk::ClearColorValue clearValue;

    // create an offscreen texture for composing the image
    VulkanAPI::Texture osTexture {};
    osTexture.create2dTex(
        *context.driver,
        vk::Format::eR32G32B32A32Sfloat,
        specularMapDim,
        specularMapDim,
        1,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled);

    ResourceBase* base = context.rGraph->getResource(specInfo.texture);
    TextureResource* tex = reinterpret_cast<TextureResource*>(base);
    VulkanAPI::Image* image = tex->get(*context.driver)->getImage();
    VulkanAPI::Image* osImage = osTexture.getImage();

    // transition cube texture for transfer
    VulkanAPI::Image::transition(
        *image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        cmdBuffer->get());

    VulkanAPI::Image::transition(
        *osImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        cmdBuffer->get());

    // record command buffer for each mip and their layers
    for (uint32_t mip = 0; mip < mipLevels; mip++)
    {
        for (uint32_t face = 0; face < 6; face++)
        {
            // get dimensions for the next mip level
            float mipDimensions = static_cast<float>(dim * std::pow(0.5, mip));

            vk::Viewport viewPort =
                vk::Viewport {0.0f, 0.0f, mipDimensions, mipDimensions, 0.0f, 1.0f};

            cmdBuffer->setViewport(viewPort);

            VulkanAPI::RenderPass* renderpass = context.rGraph->getRenderpass(context.rpass);
            cmdBuffer->bindPipeline(cbManager, renderpass, prog);
            cmdBuffer->bindDescriptors(cbManager, prog, VulkanAPI::Pipeline::Type::Graphics);
            cmdBuffer->bindVertexBuffer(skybox.vertexBuffer->get(), 0);
            cmdBuffer->bindIndexBuffer(skybox.indexBuffer->get(), 0);

            // calculate view for each cube side
            OEMaths::mat4f proj, view;
            calculateCubeTransform(face, 0.1f, 100.0f, proj, view);
            OEMaths::mat4f mvp = proj * view;

            if (type == MapType::Specular)
            {
                // push constant layout for specular map cube
                struct SpecularMapPushBlock
                {
                    OEMaths::mat4f mvp;
                    float roughness;
                    uint32_t sampleCount;
                };

                SpecularMapPushBlock pushBlock;
                pushBlock.sampleCount = 32;
                pushBlock.roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
                pushBlock.mvp = mvp;

                cmdBuffer->bindPushBlock(
                    prog,
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                    sizeof(SpecularMapPushBlock),
                    &pushBlock);
            }
            else
            {
                cmdBuffer->bindPushBlock(
                    prog, vk::ShaderStageFlagBits::eVertex, sizeof(OEMaths::mat4f), &mvp);
            }

            // draw cube into offscreen framebuffer
            cmdBuffer->drawIndexed(OESkybox::indicesSize);

            // copy the offscreen buffer to the current face
            vk::ImageSubresourceLayers src_resource(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            vk::ImageSubresourceLayers dst_resource(vk::ImageAspectFlagBits::eColor, mip, face, 1);
            vk::ImageCopy imageCopy(
                src_resource,
                {0, 0, 0},
                dst_resource,
                {0, 0, 0},
                {static_cast<uint32_t>(mipDimensions), static_cast<uint32_t>(mipDimensions), 1});

            VulkanAPI::Image::transition(
                *osImage,
                vk::ImageLayout::eColorAttachmentOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                cmdBuffer->get());
            cmdBuffer->get().copyImage(
                osImage->get(),
                vk::ImageLayout::eTransferSrcOptimal,
                image->get(),
                vk::ImageLayout::eTransferDstOptimal,
                1,
                &imageCopy);

            // transition the offscreen image back to colour attachment ready for the next image
            VulkanAPI::Image::transition(
                *osImage,
                vk::ImageLayout::eTransferSrcOptimal,
                vk::ImageLayout::eColorAttachmentOptimal,
                cmdBuffer->get());
        }
    }

    VulkanAPI::Image::transition(
        *image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        cmdBuffer->get());
}


} // namespace OmegaEngine
