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

	void TransformManager::generate_static_transform()
	{
		for (auto obj : objects) {

			uint32_t index = transformData[obj.second];
			OEMaths::mat4f mat = transformData[index].get_local();

			uint64_t parent_index = obj.first.get_parent();
			while (parent_index != UINT64_MAX) {
				mat = transformData[parent_index].get_local * mat;
			}

			transformData[obj.second].transform = mat;

			// now update all child nodes too - TODO: do this without recursion
			auto& children = obj.get_children();

			for (auto& child : children) {
				generate_static_transform();
			}
		}
	}
}
