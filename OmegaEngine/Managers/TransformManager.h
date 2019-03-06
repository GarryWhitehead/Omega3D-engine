#pragma once
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"
#include "OEMaths/OEMaths_Quat.h"
#include "Managers/ManagerBase.h"
#include "Objects/Object.h"
#include "Utility/logger.h"
#include "Vulkan/MemoryAllocator.h"

#include "tiny_gltf.h"

#include <vector>
#include <cstdint>

namespace OmegaEngine
{
	// forward decleartions
	class ObjectManager;

	class TransformManager : public ManagerBase
	{

	public:

		struct TransformData
		{
			// static 
			struct LocalTRS
			{
				OEMaths::vec3f trans;
				OEMaths::vec3f scale;
				OEMaths::mat4f rot;
			};

			OEMaths::mat4f& get_local()
			{
				return OEMaths::translate(OEMaths::mat4f(), local_trs.trans) * local_trs.rot * OEMaths::scale(OEMaths::mat4f(), local_trs.scale) * local;
			}

			LocalTRS local_trs;

			OEMaths::mat4f local;
			OEMaths::mat4f world;
			OEMaths::mat4f transform;

			// an index to skinning data for this particular node - negative number indicates no skin info
			uint32_t skin_index = -1;
		};

		// data that will be hosted on the gpu side
		struct TransformBufferInfo
		{
			OEMaths::mat4f model_matrix;
		};

		struct SkinnedBufferInfo
		{
			OEMaths::mat4f joint_matrices[256];
			uint32_t joint_count;
		};

		// skinning data derived from file 
		struct SkinInfo
		{
			const char* name;
			Object skeletonIndex;

			std::vector<Object> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
			std::vector<OEMaths::mat4f> joint_matrices;
		};

		// the number of models to allocate mem space for - this will need optimising
		// could also be dynamic and be altered to the archietecture being used
		const uint32_t TransformBlockSize = 100;
		const uint32_t SkinnedBlockSize = 100;

		TransformManager();
		~TransformManager();

		// update per frame 
		void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, std::unique_ptr<ComponentInterface>& component_interface) override;

		// gltf loading - The skinning data is going in the transform manager for now as is needed most here for calculating skinning transforms
		void addGltfSkin(tinygltf::Model& model, std::vector<Object>& linearised_objects);
		void addGltfTransform(tinygltf::Node& node, Object* obj, OEMaths::mat4f world_transform);

		// local transform and skinning update
		void update_transform(std::unique_ptr<ObjectManager>& obj_manager);
		void update_transform_recursive(uint32_t index, Object& obj, uint32_t alignment);

		// object update functions
		void update_obj_translation(Object& obj, OEMaths::vec4f trans);
		void update_obj_scale(Object& obj, OEMaths::vec4f scale);
		void update_obj_rotation(Object& obj, OEMaths::quatf rot);

		vk::Buffer& get_mesh_ubo_buffer();
		vk::Buffer& get_skinned_ubo_buffer();

		OEMaths::mat4f& get_transform(uint32_t transform_index)
		{
			assert(transform_index < transformBuffer.size());
			return transformBuffer[transform_index].transform;
		}

		uint32_t get_transform_dynamic_offsets() const
		{
			return transform_buffer->get_alignment_size();
		}

		uint32_t get_skinned_dynamic_offsets() const
		{
			return skinned_buffer->get_alignment_size();
		}

		uint32_t get_mesh_ubo_offset() const
		{
			return transform_buffer->get_offset();
		}

		uint32_t get_skinned_ubo_offset() const
		{
			return skinned_buffer->get_offset();
		}

	private:

		// transform data for static meshes
		std::vector<TransformData> transformBuffer;

		// skinned transform data
		std::vector<SkinInfo> skinBuffer;

		// transform data for each object which will be added to the GPU
		TransformBufferInfo* transform_buffer_data = nullptr;
		SkinnedBufferInfo* skinned_buffer_data = nullptr;

		// TODO: make these unique ptr
		VulkanAPI::DynamicSegment* transform_buffer;
		VulkanAPI::DynamicSegment* skinned_buffer;

		uint32_t transform_buffer_size = 0;
		uint32_t skinned_buffer_size = 0;

		// flag which tells us whether we need to update the static data
		bool is_dirty = true;
	};

}

