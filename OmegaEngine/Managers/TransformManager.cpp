#include "TransformManager.h"
#include "Objects/Object.h"
#include "Objects/ObjectManager.h"

#include "Omega_Common.h"
#include "Vulkan/BufferManager.h"
#include "Managers/EventManager.h"
#include "Managers/MeshManager.h"
#include "Engine/Omega_Global.h"
#include "Utility/GeneralUtil.h"
#include <cstdint>
#include <algorithm>

namespace OmegaEngine
{

	TransformManager::TransformManager()
	{
		transform_aligned = VulkanAPI::Util::alignment_size(sizeof(TransformBufferInfo));
		skinned_aligned = VulkanAPI::Util::alignment_size(sizeof(SkinnedBufferInfo));

		// allocate the memory used to store the transforms on the CPU side. This will be aligned as we are using dynamic buffers on the Vulkan side
		transform_buffer_data = (TransformBufferInfo*)Util::alloc_align(transform_aligned, transform_aligned * TransformBlockSize);
		skinned_buffer_data = (SkinnedBufferInfo*)Util::alloc_align(skinned_aligned, skinned_aligned * SkinnedBlockSize);
	}


	TransformManager::~TransformManager()
	{
	}

	void TransformManager::addGltfTransform(tinygltf::Node& node, Object* obj, OEMaths::mat4f world_transform)
	{
		TransformData transform;
		TransformData::LocalTRS local_trs;

		// we will save the matrix and the decomposed form
		if (node.translation.size() == 3) {
			local_trs.trans = OEMaths::convert_vec3<float>(node.translation.data());
		}
		if (node.scale.size() == 3) {
			local_trs.scale = OEMaths::convert_vec3<float>(node.scale.data());
		}
		if (node.rotation.size() == 4) {
			OEMaths::quatf q = OEMaths::convert_quatf<double>(node.rotation.data());
			local_trs.rotation = OEMaths::quat_to_mat4(q);
		}
		transform.local_trs = local_trs;

		// world transform is obtained from the omega scene file
		transform.world = world_transform;

		if (node.matrix.size() == 16) {
			transform.local = OEMaths::convert_mat4((float*)node.rotation.data());
		}
		
		// also add index to skinning information if applicable
		transform.skin_index = node.skin;

		transformBuffer.push_back(transform);

		// add to the list of entites
		obj->add_manager<TransformManager>(static_cast<uint32_t>(transformBuffer.size() - 1));
	}

	void TransformManager::addGltfSkin(tinygltf::Model& model, std::unordered_map<uint32_t, Object>& linearised_objects)
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

	void TransformManager::update_transform_recursive(std::unique_ptr<ObjectManager>& obj_manager, const uint32_t transform_index, Object& obj, uint32_t transform_alignment, uint32_t skinned_alignment)
	{
		if (obj.hasComponent<MeshManager>()) {

			TransformBufferInfo* transform_buff = transform_buffer_data + (transform_alignment * transform_index);
			SkinnedBufferInfo* skinned_buff = skinned_buffer_data + (skinned_alignment * transform_index);
			++transform_buffer_size;

			OEMaths::mat4f mat = transformBuffer[transform_index].get_local();

			uint64_t parent_id = obj.get_parent();
			while (parent_id != UINT64_MAX) {

				Object* parent_obj = obj_manager->get_object(parent_id);
				uint32_t parent_index = parent_obj->get_manager_index<TransformManager>();
				mat = transformBuffer[parent_index].get_local() * mat;
				parent_id = parent_obj->get_parent();
			}

			transformBuffer[transform_index].transform = mat;
			transform_buff->model_matrix = mat * OEMaths::scale_mat4(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });

			if (transformBuffer[transform_index].skin_index > -1) {

				++skinned_buffer_size;
				uint32_t skin_index = transformBuffer[transform_index].skin_index;

				// prepare fianl output matrices buffer
				uint64_t joint_size = static_cast<uint32_t>(skinBuffer[skin_index].joints.size()) > 256 ? 256 : skinBuffer[skin_index].joints.size();
				skinBuffer[skin_index].joint_matrices.resize(joint_size);

				skinned_buff->joint_count = joint_size;

				// transform to local space
				OEMaths::mat4f inv_mat = OEMaths::mat4_inverse(mat);

				for (uint32_t i = 0; i < joint_size; ++i) {
					Object joint_obj = skinBuffer[skin_index].joints[i];

					uint32_t joint_index = joint_obj.get_manager_index<TransformManager>();
					OEMaths::mat4f joint_mat = transformBuffer[joint_index].get_local() * skinBuffer[skin_index].invBindMatrices[i];

					// transform joint to local (joint) space
					OEMaths::mat4f local_mat = inv_mat * joint_mat;
					skinBuffer[skin_index].joint_matrices[i] = local_mat;
					skinned_buff->joint_matrices[i] = local_mat;
				}
			}
		}

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.get_children();

		for (auto& child : children) {

			// it is possible that the object has no transform, so check this first
			if (child.hasComponent<TransformManager>()) {
				update_transform_recursive(obj_manager, child.get_manager_index<TransformManager>(), child, transform_alignment, skinned_alignment);
			}
		}
	}

	void TransformManager::update_transform(std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		transform_buffer_size = 0;
		skinned_buffer_size = 0;

		for (auto obj : object_list) {

			update_transform_recursive(obj_manager, obj.second.get_manager_index<TransformManager>(), obj.second, transform_aligned, skinned_aligned);
		}
	}

	void TransformManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		// check whether static data need updating
		if (is_dirty) {

			update_transform(obj_manager);

			if (transform_buffer_size) {

				VulkanAPI::BufferUpdateEvent event{ "Transform", (void*)transform_buffer_data, transform_aligned * transform_buffer_size, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}
			if (skinned_buffer_size) {
				VulkanAPI::BufferUpdateEvent event{ "SkinnedTransform", (void*)skinned_buffer_data, skinned_aligned * skinned_buffer_size, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}

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
		transformBuffer[index].local_trs.scale = OEMaths::vec3f(scale.x, scale.y, scale.z);
		is_dirty = true;
	}

	void TransformManager::update_obj_rotation(Object& obj, OEMaths::quatf rot)
	{
		uint32_t index = obj.get_manager_index<TransformManager>();
		transformBuffer[index].local_trs.rotation = OEMaths::quat_to_mat4(rot);
		is_dirty = true;
	}
}
