#pragma once

#include "OEMaths/OEMaths.h"

#include "utility/String.h"

#include "cgltf/cgltf.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{

struct ModelNode
{
	ModelNode() = default;

	ModelNode();
	~ModelNode();

	

	OEMaths::vec3f translation;
	OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
	OEMaths::quatf rotation;

	// some models have the matrix baked already
	bool hasMatrix = false;
	OEMaths::mat4f trsMatrix;
};
} // namespace OmegaEngine
