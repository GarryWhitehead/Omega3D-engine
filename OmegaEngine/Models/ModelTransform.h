#pragma once
#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{

class ModelTransform
{

public:
	ModelTransform();
	~ModelTransform();

	void extractTransformData(tinygltf::Node &node);

	OEMaths::vec3f &getTranslation()
	{
		return translation;
	}

	OEMaths::vec3f &getScale()
	{
		return scale;
	}

	OEMaths::quatf &getRotation()
	{
		return rotation;
	}

	bool hasTrsMatrix()
	{
		return hasMatrix;
	}

	OEMaths::mat4f &getMatrix()
	{
		return trsMatrix;
	}

private:
	OEMaths::vec3f translation;
	OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
	OEMaths::quatf rotation;

	// some models have the matrix baked already
	bool hasMatrix = false;
	OEMaths::mat4f trsMatrix;
};
} // namespace OmegaEngine
