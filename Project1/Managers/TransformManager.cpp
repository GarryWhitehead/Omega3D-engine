#include "TransformManager.h"
#include "Objects/Object.h"
#include "Objects/ObjectManager.h"

#include "Omega_Common.h"

#include <cstdint>
#include <algorithm>

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
		transform.local_trs = local_trs;

		// world transform is obtained from the omega scene file
		transform.world = world_transform;

		if (node.matrix.size() == 16) {
			transform.local = OEMaths::convert_mat4((float*)node.rotation.data());
		}
		
		// also add index to skinning information in applicable
		assert(!skinBuffer.empty());
		transform.skin_index = node.skin;

		transformBuffer.push_back(transform);

		// add to the list of entites
		obj.add_manager<TransformManager>(transformBuffer.size() - 1);
	}

	void TransformManager::addGltfSkin(tinygltf::Model& model, std::vector<Object>& linearised_objects)
	{
		for (tinygltf::Skin& skin : model.skins) {
			SkinInfo skinInfo;
			skinInfo.name = skin.name.c_str();

			// Is this the skeleton root node?
			if (skin.skeleton > -1) {
				assert(skin.skeleton < linearised_objects.size());
				skinInfo.skeletonIndex = linearised_objects[skin.skeleton];
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : skin.joints) {

				// we will check later if this node actually exsists
				assert(jointIndex < linearised_objects.size() && jointIndex > -1);
				skinInfo.joints.push_back(linearised_objects[jointIndex]);
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

	void TransformManager::update_transform_recursive(uint32_t transform_index, Object& obj)
	{
		TransformBufferInfo buffer_info;
		OEMaths::mat4f mat = transformBuffer[transform_index].get_local();

		uint64_t parent_index = obj.get_parent();
		while (parent_index != UINT64_MAX) {
			mat = transformBuffer[parent_index].get_local() * mat;
		}

		transformBuffer[transform_index].transform = mat;
		buffer_info.model_matrix = mat;

		if (transformBuffer[transform_index].skin_index > -1) {
			
			uint32_t skin_index = transformBuffer[transform_index].skin_index;

			SkinnedBufferInfo skinned_info;
			// prepare fianl output matrices buffer
			uint32_t joint_size = static_cast<uint32_t>(skinBuffer[skin_index].joints.size()) > 256 ? 256 : skinBuffer[skin_index].joints.size();
			skinBuffer[skin_index].joint_matrices.resize(joint_size);
			
			skinned_info.joint_count = joint_size;

			// transform to local space
			OEMaths::mat4f inv_mat = OEMaths::inverse(mat);

			for (uint32_t i = 0; i < joint_size; ++i) {
				Object joint_obj = skinBuffer[skin_index].joints[i];

				uint32_t joint_index = joint_obj.get_manager_index<TransformManager>();
				OEMaths::mat4f joint_mat = transformBuffer[joint_index].get_local() * skinBuffer[skin_index].invBindMatrices[i];

				// transform joint to local (joint) space
				OEMaths::mat4f local_mat = inv_mat * joint_mat;
				skinBuffer[skin_index].joint_matrices[i] = local_mat;
				skinned_info.joint_matrices[i] = local_mat;
			}
		}

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.get_children();

		for (auto& child : children) {
			update_transform_recursive(child.get_manager_index<TransformManager>(), child);
		}
	}

	void TransformManager::update_transform(std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		for (auto obj : object_list) {

			update_transform_recursive(obj.second.get_manager_index<TransformManager>(), obj.second);
		}
	}

	void TransformManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager)
	{
		// check whether static data need updating
		if (is_dirty) {

			// generate new transform and skinned data for all objects
			transform_buffer_info.clear();
			skinned_buffer_info.clear();

			update_transform(obj_manager);
			is_dirty = false;
		}

	}
	
	void TransformManager::update_obj_translation(Object& obj, OEMaths::vec4f trans)
	{
		uint32_t index = obj.get_manager_index<TransformManager>();
		transformBuffer[index].local_trs.trans = OEMaths::vec3f(trans.x, trans.y, trans.z);

		// this will update all lists - though TODO: add objects which need updating for that frame to a list - should be faster?
		is_dirty = true;		
	}

	void TransformManager::update_obj_scale(Object& obj, OEMaths::vec4f scale)
	{
		uint32_t index = obj.get_manager_index<TransformManager>();
		transformBuffer[index].local_trs.trans = OEMaths::vec3f(scale.x, scale.y, scale.z);
		is_dirty = true;
	}

	void TransformManager::update_obj_rotation(Object& obj, OEMaths::quatf rot)
	{
		uint32_t index = obj.get_manager_index<TransformManager>();
		transformBuffer[index].local_trs.rot = OEMaths::quat_to_mat4(rot);
		is_dirty = true;
	}
}
