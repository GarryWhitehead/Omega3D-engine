#include "IndirectLighting.h"
#include "Core/Scene.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"
#include "Rendering/SkyboxPass.h"
#include "Types/Skybox.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Compiler/ShaderParser.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/Utility.h"
#include "utility/Logger.h"
#include "ImageUtils/MappedTexture.h"

#include <vector>

namespace OmegaEngine
{

OEIndirectLighting::OEIndirectLighting(VulkanAPI::VkDriver& driver)
    : driver(driver)
{
    init();
}

OEIndirectLighting::~OEIndirectLighting()
{
}

void OEIndirectLighting::calculateCubeTransform(
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

bool OEIndirectLighting::init()
{
    auto& manager = driver.getProgManager();
    
    // create the textures which will be sampled to.....
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(irradianceMapDim))) + 1;
    
    // bdrf
    bdrfInfo.texture = driver.add2DTexture("BdrfSampler", vk::Format::eR16G16Sfloat, bdrfDimensions, bdrfDimensions, 1, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment);
    
    // bdrf shader
    const Util::String bdrf_filename = "bdrf.glsl";
    bdrfInfo.prog = manager.getVariantOrCreate(bdrf_filename, 0);
    if (!bdrfInfo.prog)
    {
        return false;
    }
    
    if (!irradianceInfo.texture)
    {
        // irradiance
        irradianceInfo.texture = driver.add2DTexture("IrradianceSampler", irradianceFormat, irradianceMapDim, irradianceMapDim, mipLevels, 6, 1,  vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment);
        
        // irradiance shader
        const Util::String irradiance_filename = "irradianceMap.glsl";
        irradianceInfo.prog = manager.getVariantOrCreate(irradiance_filename, 0);
        if (!irradianceInfo.prog)
        {
            return false;
        }
    }
    
    if (!specularInfo.texture)
    {
        // specular
        specularInfo.texture = driver.add2DTexture("SpecularSampler", specularFormat, specularMapDim, specularMapDim, mipLevels, 6, 1,  vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment);
        
        // specular shader
        const Util::String specular_filename = "specularMap.glsl";
        specularInfo.prog = manager.getVariantOrCreate(specular_filename, 0);
        if (!specularInfo.prog)
        {
           return false;
        }
    }

    return true;
}

bool OEIndirectLighting::prepare()
{
    // either the env map must be specified or the irradiance and specular directly loaded
    // TODO: need to check in the configs whether ibl is enabled or not
    if (!envMap || (!irradianceInfo.texture && !specularInfo.texture))
    {
        LOGGER_ERROR("You must set either the environment cube map or set directly the irradiance and specular maps if indirect lighting is enabled");
        return false;
    }
        
    // brdf
    buildBdrfMap();
    
    // irradiance
    if (needsUpdate && irradianceInfo.prog)
    {
        buildMap(irradianceInfo, irradianceFormat, irradianceMapDim, MapType::Irradiance);
    }
    
    // specular
    if (needsUpdate && specularInfo.prog)
    {
        buildMap(specularInfo, specularFormat, specularMapDim, MapType::Specular);
    }
    needsUpdate = false;
    return true;
}

void OEIndirectLighting::buildBdrfMap()
{
    auto& cbManager = driver.getCbManager();
    
    // if this is the first execution of this builder, then create the vulkan backend stuff
    if (!bdrfInfo.rpass)
    {
        // create the renderpass
        std::vector<VulkanAPI::ImageView*> imageView { bdrfInfo.texture->getImageView() };
        
        bdrfInfo.rpass = std::make_unique<VulkanAPI::RenderPass>(driver.getContext());
        uint32_t handle = bdrfInfo.rpass->addOutputAttachment(bdrfFormat, 0, {}, 1);
        bdrfInfo.rpass->addSubPass(handle);
        bdrfInfo.rpass->prepare(imageView, bdrfDimensions, bdrfDimensions);
        
        // setup descriptors
        cbManager.updateShaderDescriptorSets(bdrfInfo.prog->getShaderId());
        
        // create a default pipeline
        assert(!bdrfInfo.pipeline);
        bdrfInfo.pipeline = std::make_unique<VulkanAPI::Pipeline>(driver.getContext(), *bdrfInfo.rpass, *bdrfInfo.prog->getPLineLayout(), VulkanAPI::Pipeline::Type::Graphics);
        bdrfInfo.pipeline->create(*bdrfInfo.prog);
    }
    
    VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getWorkCmdBuffer();
    
    driver.beginRenderpass(cmdBuffer, *bdrfInfo.rpass);
    cmdBuffer->bindPipeline(*bdrfInfo.pipeline);
    cmdBuffer->drawQuad();
    driver.endRenderpass(cmdBuffer);
    
    cmdBuffer->flush();
}

void OEIndirectLighting::buildMap(
    MapInfo& mapInfo,
    const vk::Format& format,
    uint32_t dim,
    const MapType type)
{
    auto& cbManager = driver.getCbManager();
    VulkanAPI::CmdBuffer* cmdBuffer = cbManager.getWorkCmdBuffer();

    vk::ClearColorValue clearValue;

    if (!mapInfo.rpass)
    {
        // create an offscreen texture for composing the image
        mapInfo.osTexture = std::make_unique<VulkanAPI::Texture>();
        mapInfo.osTexture->create2dTex(
            driver,
            format,
            dim, dim,
            1, 1, 1,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
            vk::ImageUsageFlagBits::eSampled);

        std::vector<VulkanAPI::ImageView*> imageView { mapInfo.osTexture->getImageView() };
        
        // create the renderpass
        mapInfo.rpass = std::make_unique<VulkanAPI::RenderPass>(driver.getContext());
        uint32_t handle = mapInfo.rpass->addOutputAttachment(format, 0, {}, 1, vk::ImageLayout::eColorAttachmentOptimal);
        mapInfo.rpass->addSubPass(handle);
        mapInfo.rpass->prepare(imageView, dim, dim);
        
        // setup descriptors
        cbManager.updateShaderDescriptorSets(mapInfo.prog->getShaderId());
        
        // create a default pipeline
        mapInfo.pipeline = std::make_unique<VulkanAPI::Pipeline>(driver.getContext(), *mapInfo.rpass, *mapInfo.prog->getPLineLayout(), VulkanAPI::Pipeline::Type::Graphics);
        mapInfo.pipeline->create(*mapInfo.prog);
    }
    
    VulkanAPI::Image* image = mapInfo.texture->getImage();
    VulkanAPI::Image* osImage = mapInfo.osTexture->getImage();
    
    // transition cube texture for transfer
    VulkanAPI::Image::transition(
        *osImage,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        cmdBuffer->get());

    VulkanAPI::Image::transition(
        *image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal,
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

            driver.beginRenderpass(cmdBuffer, *mapInfo.rpass);

            cmdBuffer->bindPipeline(*mapInfo.pipeline);
            cmdBuffer->bindDescriptors(cbManager, mapInfo.prog, VulkanAPI::Pipeline::Type::Graphics);
            cmdBuffer->bindVertexBuffer(envMap->vertexBuffer->get(), 0);
            cmdBuffer->bindIndexBuffer(envMap->indexBuffer->get(), 0);

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
                pushBlock.roughness = static_cast<float>(mip) / static_cast<float>(mipLevels) - 1.0f;
                pushBlock.mvp = mvp;

                cmdBuffer->bindPushBlock(
                    mapInfo.prog,
                    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                    sizeof(SpecularMapPushBlock),
                    &pushBlock);
            }
            else
            {
                cmdBuffer->bindPushBlock(
                    mapInfo.prog, vk::ShaderStageFlagBits::eVertex, sizeof(OEMaths::mat4f), &mvp);
            }

            // draw cube into offscreen framebuffer
            cmdBuffer->drawIndexed(OESkybox::indicesSize);
            driver.endRenderpass(cmdBuffer);
            
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
    
    cmdBuffer->flush();
}

void OEIndirectLighting::setEnvMap(OESkybox* skybox)
{
    assert(skybox);
    envMap = skybox;
}

void OEIndirectLighting::specularEnvMap(MappedTexture* texture)
{
    assert(texture);
    specularInfo.texture = driver.add2DTexture("SpecularSampler", VulkanAPI::VkUtil::imageFormatToVk(texture->getFormat()), texture->getWidth(), texture->getHeight(), texture->getMipLevelCount(), texture->getFaceCount(), 1, vk::ImageUsageFlagBits::eSampled);
    driver.update2DTexture("SpecularSampler", texture->getBuffer());
}

void OEIndirectLighting::irradianceEnvMap(MappedTexture* texture)
{
    assert(texture);
    irradianceInfo.texture = driver.add2DTexture("IrradianceSampler", VulkanAPI::VkUtil::imageFormatToVk(texture->getFormat()), texture->getWidth(), texture->getHeight(), texture->getMipLevelCount(), texture->getFaceCount(), 1, vk::ImageUsageFlagBits::eSampled);
    driver.update2DTexture("IrradianceSampler", texture->getBuffer());
}

bool OEIndirectLighting::needsUpdating() const
{
    return needsUpdate;
}

// =============== api functions ========================

void IndirectLighting::setEnvMap(Skybox* skybox)
{
    static_cast<OEIndirectLighting*>(this)->setEnvMap(static_cast<OESkybox*>(skybox));
}

void IndirectLighting::specularMap(MappedTexture* tex)
{
    static_cast<OEIndirectLighting*>(this)->specularEnvMap(tex);
}

void IndirectLighting::irradianceMap(MappedTexture* tex)
{
    static_cast<OEIndirectLighting*>(this)->irradianceEnvMap(tex);
}

} // namespace OmegaEngine
