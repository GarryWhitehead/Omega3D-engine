#pragma once
#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{

struct ModelTransform
{
	ModelTransform() = default;
	~ModelTransform()
	{
	}

	OEMaths::vec3f translation;
	OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
	OEMaths::quatf rotation;

	// some models have the matrix baked already
	bool hasMatrix = false;
	OEMaths::mat4f trsMatrix;
};
} // namespace OmegaEngine
