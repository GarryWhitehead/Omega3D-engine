#include "TransformManager.h"
#include "Objects/Object.h"
#include "Objects/ObjectManager.h"

#include "Omega_Common.h"
#include "Vulkan/Vulkan_Global.h"
#include "Utility/GeneralUtil.h"
#include <cstdint>
#include <algorithm>

namespace OmegaEngine
{

	TransformManager::TransformManager()
	{
		// allocate the buffers for static and skinned transforms - these will be stored in dynamic memory (host-visible) as we expect these values to be changing often
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		transform_buffer = mem_alloc.allocate_dynamic(sizeof(TransformBufferInfo), TransformBlockSize);
		skinned_buffer = mem_alloc.allocate_dynamic(sizeof(TransformBufferInfo), SkinnedBlockSize);
		
		assert(transform_buffer != nullptr && skinned_buffer != nullptr);
	
		// allocate the memory used to store the transforms on the CPU side. This will be aligned as we are using dynamic buffers on the Vulkan side#
		transform_buffer_data = (TransformBufferInfo*)Util::alloc_align(transform_buffer->get_alignment_size(), transform_buffer->get_alignment_size() * TransformBlockSize);
		skinned_buffer_data = (SkinnedBufferInfo*)Util::alloc_align(skinned_buffer->get_alignment_size(), skinned_buffer->get_alignment_size() * SkinnedBlockSize);
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
			local_trs.rot = OEMaths::quat_to_mat4(q);
		}
		transform.local_trs = local_trs;

		// world transform is obtained from the omega scene file
		transform.world = world_transform;

		if (node.matrix.size() == 16) {
			transform.local = OEMaths::convert_mat4((float*)node.rotation.data());
		}
		
		// also add index to skinning information in applicable
		transform.skin_index = node.skin;

		transformBuffer.push_back(transform);

		// add to the list of entites
		obj->add_manager<TransformManager>(static_cast<uint32_t>(transformBuffer.size() - 1));
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

	void TransformManager::update_transform_recursive(uint32_t transform_index, Object& obj, uint32_t alignment)
	{
		TransformBufferInfo* transform_buff = transform_buffer_data + (alignment * transform_index);
		SkinnedBufferInfo* skinned_buff = skinned_buffer_data + (alignment * transform_index);
		++transform_buffer_size;

		OEMaths::mat4f mat = transformBuffer[transform_index].get_local();

		uint64_t parent_index = obj.get_parent();
		while (parent_index != UINT64_MAX) {
			mat = transformBuffer[parent_index].get_local() * mat;
		}

		transformBuffer[transform_index].transform = mat;
		transform_buff->model_matrix = mat;

		if (transformBuffer[transform_index].skin_index > -1) {
			
			++skinned_buffer_size;
			uint32_t skin_index = transformBuffer[transform_index].skin_index;

			// prepare fianl output matrices buffer
			uint32_t joint_size = static_cast<uint32_t>(skinBuffer[skin_index].joints.size()) > 256 ? 256 : skinBuffer[skin_index].joints.size();
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

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.get_children();

		for (auto& child : children) {
			update_transform_recursive(child.get_manager_index<TransformManager>(), child, alignment);
		}
	}

	void TransformManager::update_transform(std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		for (auto obj : object_list) {

			update_transform_recursive(obj.second.get_manager_index<TransformManager>(), obj.second, transform_buffer->get_alignment_size());
		}
	}

	void TransformManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		// check whether static data need updating
		if (is_dirty) {

			update_transform(obj_manager);

			// now upload to gpu - TODO: note that this will need to be altered so only models that have 'dirty' data are updated
			VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
			if (transform_buffer_size) {
				mem_alloc.mapDataToDynamicSegment(transform_buffer, transform_buffer_data, transform_buffer->get_alignment_size() * transform_buffer_size);
			}
			if (skinned_buffer_size) {
				mem_alloc.mapDataToDynamicSegment(skinned_buffer, skinned_buffer_data, skinned_buffer->get_alignment_size() * skinned_buffer_size);
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
		transformBuffer[index].local_trs.trans = OEMaths::vec3f(scale.x, scale.y, scale.z);
		is_dirty = true;
	}

	void TransformManager::update_obj_rotation(Object& obj, OEMaths::quatf rot)
	{
		uint32_t index = obj.get_manager_index<TransformManager>();
		transformBuffer[index].local_trs.rot = OEMaths::quat_to_mat4(rot);
		is_dirty = true;
	}

	vk::Buffer& TransformManager::get_mesh_ubo_buffer()
	{
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		return mem_alloc.get_memory_buffer(transform_buffer->get_block_id());
	}

	vk::Buffer& TransformManager::get_skinned_ubo_buffer()
	{
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		return mem_alloc.get_memory_buffer(skinned_buffer->get_block_id());
	}
}
