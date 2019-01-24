#include "TransformManager.h"
#include "DataTypes/Object.h"
#include "ComponentInterface/ObjectManager.h"

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
		obj.add_manager<TransformManager>(transformBuffer.size() - 1);
	}

	void TransformManager::update_static_recursive(uint32_t transform_index, Object& obj)
	{
		OEMaths::mat4f mat = transformBuffer[transform_index].get_local();

		uint64_t parent_index = obj.get_parent();
		while (parent_index != UINT64_MAX) {
			mat = transformBuffer[parent_index].get_local * mat;
		}

		transformBuffer[transform_index].set_transform(mat);

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.get_children();

		for (auto& child : children) {
			update_static_recursive(child.get_manager_index<TransformManager>(), child);
		}
	}

	void TransformManager::update_static(std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		for (auto obj : object_list) {

			update_static_recursive(obj.second.get_manager_index<TransformManager>(), obj.second);
		}
	}
}
