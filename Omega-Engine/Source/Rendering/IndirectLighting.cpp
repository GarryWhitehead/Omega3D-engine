#include "IndirectLighting.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"
#include "Rendering/SkyboxPass.h"
#include "Types/Skybox.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Sampler.h"
#include "VulkanAPI/Shader.h"
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
    OEMaths::vec3f target[6] = {OEMaths::vec3f(1.0f, 0.0f, 0.0f),
                                OEMaths::vec3f(-1.0f, 0.0f, 0.0f),
                                OEMaths::vec3f(0.0f, 1.0f, 0.0f),
                                OEMaths::vec3f(0.0f, -1.0f, 0.0f),
                                OEMaths::vec3f(0.0f, 0.0f, 1.0f),
                                OEMaths::vec3f(0.0f, 0.0f, -1.0f)};

    OEMaths::vec3f cameraUp[6] = {OEMaths::vec3f(0.0f, 1.0f, 0.0f),
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

bool IndirectLighting::prepare(VulkanAPI::ProgramManager* manager)
{
    const uint32_t lutDimensions = 512;
    vk::ClearColorValue clearValue;

    // bdrf
    {
        VulkanAPI::ProgramManager::ShaderHash key = {"bdrf.json", 0, nullptr};
        VulkanAPI::ShaderProgram* prog = manager->getVariant(key);
        if (!prog)
        {
            return false;
        }

        RenderGraphBuilder builder =
            rGraph.createPass("BdrfGenerationPass", RenderGraphPass::Type::Graphics);

        bdrfInfo.texture =
            builder.createTexture(lutDimensions, lutDimensions, vk::Format::eR16G16Sfloat);

        bdrfInfo.attachment = builder.addOutputAttachment("BdrfSampler", bdrfInfo.texture);

        builder.addExecute([=](RGraphContext& context) {
            VulkanAPI::CmdBuffer* cmdBuffer = context.cmdBuffer;
            cmdBuffer->bindPipeline(context.rpass, prog);
            cmdBuffer->drawQuad();
        });
    }

    // irradiance
    {
        VulkanAPI::ProgramManager::ShaderHash key = {"irradiance.glsl", 0, nullptr};
        VulkanAPI::ShaderProgram* prog = manager->getVariant(key);
        if (!prog)
        {
            return false;
        }

        RenderGraphBuilder builder =
            rGraph.createPass("IrradiancePass", RenderGraphPass::Type::Graphics);

        irrInfo.texture = builder.createTexture(
            irradianceMapDim, irradianceMapDim, vk::Format::eR32G32B32A32Sfloat);

        irrInfo.attachment = builder.addOutputAttachment("IrradianceSampler", irrInfo.texture);

        builder.addExecute([=](RGraphContext& context) {
            buildMap(context, prog, irradianceMapDim, MapType::Irradiance, skybox);
        });
    }

    // specular
    {
        VulkanAPI::ProgramManager::ShaderHash key = {"specular.glsl", 0, nullptr};
        VulkanAPI::ShaderProgram* prog = manager->getVariant(key);
        if (!prog)
        {
            return false;
        }

        RenderGraphBuilder builder =
            rGraph.createPass("SpecularPass", RenderGraphPass::Type::Graphics);

        specInfo.texture =
            builder.createTexture(specularMapDim, specularMapDim, vk::Format::eR32G32B32A32Sfloat);

        specInfo.attachment = builder.addOutputAttachment("SpecularSampler", specInfo.texture);

        builder.addExecute([=](RGraphContext& context) {
            buildMap(context, prog, specularMapDim, MapType::Specular, skybox);
        });
    }

    return true;
}

void IndirectLighting::buildMap(
    RGraphContext& context,
    VulkanAPI::ShaderProgram* prog,
    uint32_t dim,
    const MapType type,
    OESkybox& skybox)
{
    vk::ClearColorValue clearValue;
    VulkanAPI::CmdBuffer* cmdBuffer = context.cmdBuffer;

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
    VulkanAPI::Image* image = tex->get()->getImage();
    VulkanAPI::Image* osImage = osTexture.getImage();

    // transition cube texture for transfer
    VulkanAPI::Image::transition(
        *image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
        context.cmdBuffer->get());

    VulkanAPI::Image::transition(
        *osImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        context.cmdBuffer->get());

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

            cmdBuffer->bindPipeline(context.rpass, prog);
            cmdBuffer->bindDescriptors(prog, VulkanAPI::Pipeline::Type::Graphics);
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
