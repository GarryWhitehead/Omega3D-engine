#pragma once
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward decleartions
	class Object;

	class TransformManager
	{

	public:

		struct TransformData
		{
			struct LocalDecomposition
			{
				OEMaths::vec3f trans;
				OEMaths::vec3f scale;
				OEMaths::mat4f rot;

			} local_decomposed;

			OEMaths::mat4f local;
			OEMaths::mat4f world;
		};

		TransformManager();
		~TransformManager();

		void addGltfTransform(tinygltf::Node& node, Object& obj, OEMaths::mat4f world_transform);

	private:

		std::vector<TransformData> transformBuffer;
	};

}

