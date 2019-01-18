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
		TransformData::LocalTRS local_trs;

		// we will save the matrix and the decomposed form
		if (node.translation.size() == 3) {
			local_trs.trans = OEMaths::convert_vec3((float*)node.translation.data());
		}
		if (node.scale.size() == 3) {
			local_trs.scale = OEMaths::convert_vec3((float*)node.scale.data());
		}
		if (node.rotation.size() == 4) {
			OEMaths::quatf q = OEMaths::convert_quat((float*)node.rotation.data());
			local_trs.rot = OEMaths::quat_to_mat4(q);
		}
		transform.set_trs(local_trs);

		// world transform is obtained from the omega scene file
		transform.set_world(world_transform);

		if (node.matrix.size() == 16) {
			transform.set_local(OEMaths::convert_mat4((float*)node.rotation.data()));
		}
		transformBuffer.push_back(transform);

		// add to the list of entites
		objects[obj] = transformBuffer.size() - 1;
	}

	void TransformManager::generate_static_transform()
	{
		for (auto obj : objects) {

			Object object = obj.first;

			OEMaths::mat4f mat = transformBuffer[obj.second].get_local();

			uint64_t parent_index = object.get_parent();
			while (parent_index != UINT64_MAX) {
				mat = transformBuffer[parent_index].get_local * mat;
			}

			transformBuffer[obj.second].set_transform(mat);

			// now update all child nodes too - TODO: do this without recursion
			auto& children = object.get_children();

			for (auto& child : children) {
				generate_static_transform();
			}
		}
	}
}
