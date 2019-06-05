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
		
		void extractTransformData(tinygltf::Node& node);

	private:
		
		OEMaths::vec3f translation;
		OEMaths::vec3f scale;
		OEMaths::quatf rotation;
		OEMaths::mat4f trsMatrix;
	};
}

