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

#include "omega-engine/Skybox.h"

#include <cstdint>

// vulkan forward declerations
namespace VulkanAPI
{
class VertexBuffer;
class IndexBuffer;
class VkDriver;
} // namespace VulkanAPI

namespace OmegaEngine
{
// forward declerations
class OEIndirectLighting;
class SkyboxPass;
class MappedTexture;

class OESkybox : public Skybox
{
public:
    static constexpr uint32_t indicesSize = 36;
    static constexpr uint32_t verticesSize = 24;

    OESkybox(VulkanAPI::VkDriver& driver);
    ~OESkybox();

    /**
     * @brief Prepares the geomtry required for drawing the skybox and pushes it to the vulkan
     * backend
     */
    void prepareGeometry();

    OESkybox& setCubeMap(MappedTexture* cm);
    OESkybox& setBlurFactor(const float bf);

    friend class OEIndirectLighting;
    friend class SkyboxPass;
    friend class OEScene;

private:
    VulkanAPI::VkDriver& driver;

    // the cubemap
    MappedTexture* cubeMap = nullptr;

    // vertex and index buffer memory info for the cube
    VulkanAPI::VertexBuffer* vertexBuffer = nullptr;
    VulkanAPI::IndexBuffer* indexBuffer = nullptr;
    uint32_t indexCount = 0;

    // the factor which the skybox will be blurred by
    float blurFactor = 0.0f;
};

} // namespace OmegaEngine
