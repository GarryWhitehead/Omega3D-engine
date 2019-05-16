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
		if (transform_buffer_data) 
		{
			_aligned_free(transform_buffer_data);
		}
		if (skinned_buffer_data) 
		{
			_aligned_free(skinned_buffer_data);
		}
	}

	void TransformManager::addGltfTransform(tinygltf::Node& node, Object* obj, OEMaths::mat4f world_transform)
	{
		TransformData transform;

		// we will save the matrix and the decomposed form
		if (node.translation.size() == 3) 
		{
			transform.set_translation(OEMaths::vec3f{ (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] });
		}
		if (node.scale.size() == 3) 
		{
			transform.set_scale(OEMaths::vec3f{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] });
		}
		if (node.rotation.size() == 4) 
		{
			OEMaths::quatf quat(node.rotation.data());
			transform.set_rotation(quat);
		}

		// world transform is obtained from the omega scene file
		transform.set_world_matrix(world_transform);

		if (node.matrix.size() == 16) 
		{
			transform.set_local_matrix(OEMaths::mat4f(node.matrix.data()));
		}
		
		// also add index to skinning information if applicable
		transform.set_skin_index(node.skin);

		transformBuffer[obj->get_id()] = transform;

		// add to the list of entites
		obj->add_component<TransformComponent>(static_cast<uint32_t>(transformBuffer.size() - 1));
	}

	void TransformManager::addGltfSkin(tinygltf::Model& model, std::unordered_map<uint32_t, Object>& linearised_objects)
	{
		for (tinygltf::Skin& skin : model.skins) 
		{
			SkinInfo skinInfo;
			skinInfo.name = skin.name.c_str();

			// Is this the skeleton root node?
			if (skin.skeleton > -1) 
			{
				assert(skin.skeleton < linearised_objects.size());
				skinInfo.skeletonIndex = linearised_objects[skin.skeleton];
				skinInfo.skeletonIndex.add_component<SkinnedComponent>(static_cast<uint32_t>(skinBuffer.size() - 1));
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : skin.joints) 
			{
				// we will check later if this node actually exsists
				assert(jointIndex < linearised_objects.size() && jointIndex > -1);
				skinInfo.joints.push_back(linearised_objects[jointIndex]);
			}

			// get the inverse bind matricies, if there are any
			if (skin.inverseBindMatrices > -1) 
			{
				tinygltf::Accessor accessor = model.accessors[skin.inverseBindMatrices];
				tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

				skinInfo.invBindMatrices.resize(accessor.count);
				memcpy(skinInfo.invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
			}

			skinBuffer.push_back(skinInfo);
		}
	}

	OEMaths::mat4f TransformManager::create_matrix(Object& obj, std::unique_ptr<ObjectManager>& obj_manager)
	{
		OEMaths::mat4f mat = transformBuffer[obj.get_id()].get_local_matrix();

		uint64_t parent_id = obj.get_parent();
		while (parent_id != UINT64_MAX) 
		{
			Object* parent_obj = obj_manager->get_object(parent_id);

			uint32_t id = parent_obj->get_id();
			mat = transformBuffer[id].get_local_matrix() * mat;
			parent_id = parent_obj->get_parent();
		}

		return mat;
	}

	void TransformManager::update_transform_recursive(std::unique_ptr<ObjectManager>& obj_manager, Object& obj, uint32_t transform_alignment, uint32_t skinned_alignment)
	{
		if (obj.hasComponent<MeshComponent>() && obj.hasComponent<SkinnedComponent>()) 
		{
			uint32_t id = obj.get_id();

			TransformBufferInfo* transform_buff = (TransformBufferInfo*)((uint64_t)transform_buffer_data + (transform_alignment * transform_buffer_size));
			SkinnedBufferInfo* skinned_buff = (SkinnedBufferInfo*)((uint64_t)skinned_buffer_data + (skinned_alignment * skinned_buffer_size));
			
			transformBuffer[id].set_transform_offset(transform_buffer_size * transform_alignment);

			OEMaths::mat4f mat = create_matrix(obj, obj_manager);
			transform_buff->model_matrix = mat * OEMaths::mat4f::scale(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });
			
			++transform_buffer_size;

			// skinned transform
			transformBuffer[id].set_skinned_offset(skinned_buffer_size * skinned_alignment);

			uint32_t skin_index = transformBuffer[id].get_skin_index();

			// prepare fianl output matrices buffer
			uint64_t joint_size = static_cast<uint32_t>(skinBuffer[skin_index].joints.size()) > 256 ? 256 : skinBuffer[skin_index].joints.size();
			skinBuffer[skin_index].joint_matrices.resize(joint_size);

			skinned_buff->joint_count = joint_size;

			// transform to local space
			OEMaths::mat4f inv_mat = mat.inverse();

			for (uint32_t i = 0; i < joint_size; ++i) 
			{	
				Object joint_obj = skinBuffer[skin_index].joints[i];
				OEMaths::mat4f joint_mat = create_matrix(joint_obj, obj_manager) * skinBuffer[skin_index].invBindMatrices[i];

				// transform joint to local (joint) space
				OEMaths::mat4f local_mat = inv_mat * joint_mat;
				skinBuffer[skin_index].joint_matrices[i] = local_mat;
				skinned_buff->joint_matrices[i] = local_mat;
			}
			++skinned_buffer_size;
	
		}
		else if (obj.hasComponent<MeshComponent>())
		{
			uint32_t id = obj.get_id();

			TransformBufferInfo* transform_buff = (TransformBufferInfo*)((uint64_t)transform_buffer_data + (transform_alignment * transform_buffer_size));
			
			transformBuffer[id].set_transform_offset(transform_buffer_size * transform_alignment);

			OEMaths::mat4f mat = create_matrix(obj, obj_manager);
			transform_buff->model_matrix = mat * OEMaths::mat4f::scale(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });
			
			++transform_buffer_size;
		}

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.get_children();

		for (auto& child : children) 
		{
			// it is possible that the object has no transform, so check this first
			if (child.hasComponent<TransformManager>()) {
				update_transform_recursive(obj_manager, child, transform_alignment, skinned_alignment);
			}
		}
	}

	void TransformManager::update_transform(std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		transform_buffer_size = 0;
		skinned_buffer_size = 0;

		for (auto obj : object_list)
		 {
			update_transform_recursive(obj_manager, obj.second, transform_aligned, skinned_aligned);
		}
	}

	void TransformManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		// check whether static data need updating
		if (is_dirty) 
		{
			update_transform(obj_manager);

			if (transform_buffer_size) 
			{
				VulkanAPI::BufferUpdateEvent event{ "Transform", (void*)transform_buffer_data, transform_aligned * transform_buffer_size, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}
			if (skinned_buffer_size) 
			{
				VulkanAPI::BufferUpdateEvent event{ "SkinnedTransform", (void*)skinned_buffer_data, skinned_aligned * skinned_buffer_size, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}

			is_dirty = false;
		}
	}
	
	void TransformManager::update_obj_translation(Object& obj, OEMaths::vec4f trans)
	{
		uint32_t id = obj.get_id();
		transformBuffer[id].set_translation(OEMaths::vec3f{ trans.getX(), trans.getY(), trans.getZ() });

		// this will update all lists - though TODO: add objects which need updating for that frame to a list - should be faster?
		is_dirty = true;		
	}

	void TransformManager::update_obj_scale(Object& obj, OEMaths::vec4f scale)
	{
		uint32_t id = obj.get_id();
		transformBuffer[id].set_scale(OEMaths::vec3f{ scale.getX(), scale.getY(), scale.getZ() });
		is_dirty = true;
	}

	void TransformManager::update_obj_rotation(Object& obj, OEMaths::quatf rot)
	{
		uint32_t id = obj.get_id();
		transformBuffer[id].set_rotation(rot);
		is_dirty = true;
	}
}
