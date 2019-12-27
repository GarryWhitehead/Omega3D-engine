
#include "Skybox.h"

#include "OEMaths/OEMaths.h"

#include "VulkanAPI/VkDriver.h"

#include <array>

namespace OmegaEngine
{

Skybox::Skybox(VulkanAPI::VkDriver& driver) :
    driver(driver)
{
}

Skybox::~Skybox()
{
}

void Skybox::prepareGeometry()
{
    // cube vertices
    static std::array<OEMaths::vec3f, verticesSize> vertices{

        OEMaths::vec3f{ -1.0f, -1.0f, 1.0f },  OEMaths::vec3f{ 1.0f, -1.0f, 1.0f },
        OEMaths::vec3f{ 1.0f, 1.0f, 1.0f },    OEMaths::vec3f{ -1.0f, 1.0f, 1.0f },
        OEMaths::vec3f{ -1.0f, -1.0f, -1.0f }, OEMaths::vec3f{ 1.0f, -1.0f, -1.0f },
        OEMaths::vec3f{ 1.0f, 1.0f, -1.0f },   OEMaths::vec3f{ -1.0f, 1.0f, -1.0f }
    };

    // cube indices
    static std::array<uint32_t, indicesSize> indices{ // front
                                                      0, 1, 2, 2, 3, 0,
                                                      // right side
                                                      1, 5, 6, 6, 2, 1,
                                                      // back
                                                      7, 6, 5, 5, 4, 7,
                                                      // left side
                                                      4, 0, 3, 3, 7, 4,
                                                      // bottom
                                                      4, 5, 1, 1, 0, 4,
                                                      // top
                                                      3, 2, 6, 6, 7, 3
    };

    // create vertex buffer
    vertexBuffer = driver.addVertexBuffer(verticesSize * sizeof(OEMaths::vec3f), vertices.data());
    
    // and the index buffer
    indexBuffer = driver.addIndexBuffer(indicesSize * sizeof(uint32_t), indices.data());

    // only draw the skybox where there is no geometry
    /*state->pipeline.setStencilStateFrontAndBack(vk::CompareOp::eNotEqual, vk::StencilOp::eKeep, vk::StencilOp::eKeep,
                                                vk::StencilOp::eReplace, 0xff, 0x00, 1);

    state->pipeline.setDepthState(VK_FALSE, VK_FALSE, vk::CompareOp::eLessOrEqual);
    state->pipeline.setRasterCullMode(vk::CullModeFlagBits::eNone);*/
}

}
