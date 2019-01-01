#include "TransformManager.h"
#include "DataTypes/Object.h"

namespace OmegaEngine
{

	TransformManager::TransformManager()
	{
	}


	TransformManager::~TransformManager()
	{
	}

	uint32_t TransformManager::addGltfTransform(tinygltf::Node& node, Object& obj, OEMaths::mat4f world_transform)
	{
		TransformData transform;
		
		// we will save the matrix and the decomposed form
		if (node.translation.size() == 3) {
			transform.local_decomposed.trans = OEMaths::convert_vec3((float*)node.translation.data());
		}
		if (node.scale.size() == 3) {
			transform.local_decomposed.scale = OEMaths::convert_vec3((float*)node.scale.data());
		}
		if (node.rotation.size() == 4) {
			OEMaths::quatf q = OEMaths::convert_quat((float*)node.rotation.data());
			transform.local_decomposed.rot = OEMaths::quat_to_mat4(q);
		}

		// world transform is obtained from the omega scene file
		transform.world = world_transform;

		if (node.matrix.size() == 16) {
			transform.local = OEMaths::convert_mat4((float*)node.rotation.data());
		}
		transformBuffer.push_back(transform);

		// add to the list of entites
		objects[obj] = transformBuffer.size() - 1;
	}
}
