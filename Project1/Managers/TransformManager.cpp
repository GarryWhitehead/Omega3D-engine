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

	void TransformManager::addGltfSkin(tinygltf::Model& model)
	{
		for (tinygltf::Skin& skin : model.skins) {
			SkinInfo skinInfo;
			skinInfo.name = skin.name.c_str();

			// Is this the skeleton root node?
			if (skin.skeleton > -1) {
				skinInfo.skeletonIndex = skin.skeleton;
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : skin.joints) {

				// we will check later if this node actually exsists
				skinInfo.joints.push_back(jointIndex);
			}

			// get the inverse bind matricies, if there are any
			if (skin.inverseBindMatrices > -1) {
				tinygltf::Accessor accessor = model.accessors[skin.inverseBindMatrices];
				tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

				skinInfo.invBindMatrices.resize(accessor.count);
				memcpy(skinInfo.invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
			}

			skinBuffer.push_back(skinInfo);
		}
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

	void TransformManager::update_skinned_recursive(std::unique_ptr<ObjectManager>& obj_manager, uint32_t anim_index, Object& obj)
	{

		uint32_t transform_index = obj.get_manager_index<TransformManager>();
		OEMaths::mat4f mat = transformBuffer[transform_index].get_local();

		// prepare fianl output matrices buffer
		uint32_t joint_size = std::min(skinBuffer[anim_index].joints.size(), MAX_NUM_JOINTS);
		skinBuffer[anim_index].joint_matrices.resize(joint_size);

		// transform to local space
		OEMaths::mat4f inv_mat = OEMaths::inverse(mat);


		for (uint32_t i = 0; i < joint_size; ++i) {
			Object joint_obj = skinBuffer[anim_index].joints[i];

			uint32_t joint_index = joint_obj.get_manager_index<TransformManager>();
			OEMaths::mat4f joint_mat = transformBuffer[joint_index].get_local() * skinBuffer[anim_index].invBindMatrices[i];

			// transform joint to local (joint) space
			skinBuffer[anim_index].joint_matrices[i] = inv_mat * joint_mat;
		}

		// now update all child nodes too - TODO: do this without recursion
		auto& children = obj.get_children();

		for (auto& child : children) {
			update_skinned_recursive(obj_manager, child.get_manager_index<AnimationManager>(), child);
		}

	}

	void TransformManager::update_skinned(std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		for (auto obj : object_list) {

			update_skinned_recursive(obj_manager, obj.second.get_manager_index<AnimationManager>(), obj.second);
		}


	}

	void TransformManager::update_frame(double time, double dt)
	{
		// check whether static data need updating
		if (is_static_dirty) {

			update_static();
			is_static_dirty = false;
		}

		if (is_skinned_dirty) {

			update_skinned();
			is_skinned_dirty = false;
		}
	}
}
