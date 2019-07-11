#pragma once

#include "OEMaths/OEMaths.h"
#include "Models/ModelMesh.h"

#include <memory>
#include <cstdint>

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

std::unique_ptr<OmegaEngine::ModelMesh> generatePlaneMesh(const uint32_t size,
                                                           const uint32_t uvFactor);
std::unique_ptr<OmegaEngine::ModelMesh> generateSphereMesh(const uint32_t density);
std::unique_ptr<OmegaEngine::ModelMesh> generateCubeMesh(const OEMaths::vec3f &size);

} // namespace Models
} // namespace OmegaEngine