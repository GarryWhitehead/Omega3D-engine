#pragma once

#include "omega-engine/Skybox.h"

#include <cstdint>

// vulkan forward declerations
namespace VulkanAPI
{
class VertexBuffer;
class IndexBuffer;
class VkDriver;
}

namespace OmegaEngine
{
// forward declerations
class IndirectLighting;
class SkyboxPass;
class MappedTexture;

class OESkybox : public Skybox
{
public:
    
    static constexpr uint32_t indicesSize = 36;
    static constexpr uint32_t verticesSize = 24;
    
    OESkybox(VulkanAPI::VkDriver& driver, MappedTexture* cm, float blur = 0.0f);
    ~OESkybox();
    
    /**
     * @brief Prepares the geomtry required for drawing the skybox and pushes it to the vulkan backend
     */
    void prepareGeometry();
    
    friend class IndirectLighting;
    friend class SkyboxPass;
    
private:
    
    VulkanAPI::VkDriver& driver;
    
    // the cubemap
    MappedTexture* cubeMap;
    
    // vertex and index buffer memory info for the cube
    VulkanAPI::VertexBuffer* vertexBuffer = nullptr;
    VulkanAPI::IndexBuffer* indexBuffer = nullptr;
    uint32_t indexCount = 0;

    // the factor which the skybox will be blurred by
    float blurFactor = 0.0f;
};

}
