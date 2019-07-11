#pragma once
#include "OEMaths/OEMaths.h"

#include <memory>

namespace OmegaEngine
{

struct ModelTransform
{
	ModelTransform() = default;

	ModelTransform(const OEMaths::vec3f& trans, const OEMaths::vec3f& sca, const OEMaths::quatf& rot) :
		translation(trans),
		scale(sca),
		rotation(rot)
	{
	}

	~ModelTransform()
	{
	}

	OEMaths::vec3f translation;
	OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
	OEMaths::quatf rotation;

	// some models have the matrix baked already
	bool hasMatrix = false;
	OEMaths::mat4f trsMatrix;
};
} // namespace OmegaEngine
