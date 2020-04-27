#pragma once

#include "Models/ModelMesh.h"
#include "OEMaths/OEMaths.h"

#include <cstdint>
#include <memory>

namespace OmegaEngine
{
namespace OEModels
{

enum class Type
{
    Cube,
    Plane,
    Sphere
};

std::unique_ptr<OmegaEngine::ModelMesh> generateQuadMesh(const float size);
std::unique_ptr<OmegaEngine::ModelMesh> generateSphereMesh(const uint32_t density);
std::unique_ptr<OmegaEngine::ModelMesh>
generateCapsuleMesh(const uint32_t density, const float height, const float width);
std::unique_ptr<OmegaEngine::ModelMesh> generateCubeMesh(const OEMaths::vec3f& size);

} // namespace OEModels
} // namespace OmegaEngine