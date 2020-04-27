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

#include "Skybox.h"

#include "ImageUtils/MappedTexture.h"
#include "OEMaths/OEMaths.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"

#include <array>

namespace OmegaEngine
{

// ==============================================================================

OESkybox::OESkybox(VulkanAPI::VkDriver& driver) : driver(driver)
{
    prepareGeometry();
}

OESkybox::~OESkybox()
{
}

void OESkybox::prepareGeometry()
{
    // cube vertices
    static std::array<OEMaths::vec3f, verticesSize> vertices {

        OEMaths::vec3f {-1.0f, -1.0f, 1.0f},
        OEMaths::vec3f {1.0f, -1.0f, 1.0f},
        OEMaths::vec3f {1.0f, 1.0f, 1.0f},
        OEMaths::vec3f {-1.0f, 1.0f, 1.0f},
        OEMaths::vec3f {-1.0f, -1.0f, -1.0f},
        OEMaths::vec3f {1.0f, -1.0f, -1.0f},
        OEMaths::vec3f {1.0f, 1.0f, -1.0f},
        OEMaths::vec3f {-1.0f, 1.0f, -1.0f}};

    // cube indices
    static std::array<uint32_t, indicesSize> indices {// front
                                                      0,
                                                      1,
                                                      2,
                                                      2,
                                                      3,
                                                      0,
                                                      // right side
                                                      1,
                                                      5,
                                                      6,
                                                      6,
                                                      2,
                                                      1,
                                                      // back
                                                      7,
                                                      6,
                                                      5,
                                                      5,
                                                      4,
                                                      7,
                                                      // left side
                                                      4,
                                                      0,
                                                      3,
                                                      3,
                                                      7,
                                                      4,
                                                      // bottom
                                                      4,
                                                      5,
                                                      1,
                                                      1,
                                                      0,
                                                      4,
                                                      // top
                                                      3,
                                                      2,
                                                      6,
                                                      6,
                                                      7,
                                                      3};

    // create vertex buffer
    vertexBuffer = driver.addVertexBuffer(verticesSize * sizeof(OEMaths::vec3f), vertices.data());
    assert(vertexBuffer);

    // and the index buffer
    indexBuffer = driver.addIndexBuffer(indicesSize * sizeof(uint32_t), indices.data());
    assert(indexBuffer);

    // only draw the skybox where there is no geometry
    /*state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eNotEqual, vk::StencilOp::eKeep,
vk::StencilOp::eKeep, vk::StencilOp::eReplace, 0xff, 0x00, 1);

state->pipeline.setDepthState(VK_FALSE, VK_FALSE, vk::CompareOp::eLessOrEqual);
state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eNone);*/
}

OESkybox& OESkybox::setCubeMap(MappedTexture* cm)
{
    assert(cm);
    cubeMap = cm;
    VulkanAPI::Texture* tex = driver.findOrCreateTexture2d(
        "envSampler",
        VulkanAPI::VkUtil::imageFormatToVk(cubeMap->getFormat()),
        cubeMap->getWidth(),
        cubeMap->getHeight(),
        cubeMap->getMipLevelCount(),
        cubeMap->getFaceCount(),
        1,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment);

    tex->map(driver, cubeMap->getBuffer());

    return *this;
}

OESkybox& OESkybox::setBlurFactor(const float bf)
{
    blurFactor = std::clamp(bf, 0.0f, 1.0f);
    return *this;
}

// =================== front-end ==============================

Skybox& Skybox::setCubeMap(MappedTexture* cm)
{
    return static_cast<OESkybox*>(this)->setCubeMap(cm);
}

Skybox& Skybox::setBlurFactor(const float bf)
{
    return static_cast<OESkybox*>(this)->setBlurFactor(bf);
}

} // namespace OmegaEngine
