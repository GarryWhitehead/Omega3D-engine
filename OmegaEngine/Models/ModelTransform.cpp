#include "ModelTransform.h"

namespace OmegaEngine
{

	ModelTransform::ModelTransform()
	{
	}


	ModelTransform::~ModelTransform()
	{
	}

	void ModelTransform::extractTransformData(tinygltf::Node& node)
	{
		// we will save the matrix and the decomposed form
		if (node.translation.size() == 3)
		{
			this->translation = OEMaths::vec3f{ (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };
		}
		if (node.scale.size() == 3)
		{
			this->scale = OEMaths::vec3f{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
		}
		if (node.rotation.size() == 4)
		{
			OEMaths::quatf quat(node.rotation.data());
			this->rotation = quat;
		}

		if (node.matrix.size() == 16)
		{
			trsMatrix = OEMaths::mat4f(node.matrix.data());
		}
	}
}
