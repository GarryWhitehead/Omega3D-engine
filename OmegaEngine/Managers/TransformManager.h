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
				OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
				OEMaths::mat4f rotation;
			};

			void calculate_local()
			{
				local = OEMaths::translate_mat4(local_trs.trans) * local_trs.rotation * OEMaths::scale_mat4(local_trs.scale);
			}

			uint32_t get_transform_offset() const
			{
				return transform_buffer_offset;
			}

			uint32_t get_skinned_offset() const
			{
				return skinned_buffer_offset;
			}

			void set_transform_offset(const uint32_t offset)
			{
				transform_buffer_offset = offset;
			}

			void set_skinned_offset(const uint32_t offset)
			{
				skinned_buffer_offset = offset;
			}

			void set_translation(OEMaths::vec3f& trans)
			{
				local_trs.trans = trans;
				recalculate_local = true;
			}

			void set_rotation(OEMaths::mat4f& rot)
			{
				local_trs.rotation = rot;
				recalculate_local = true;
			}

			void set_rotation(OEMaths::quatf& q)
			{
				local_trs.rotation = OEMaths::quat_to_mat4(q);
				recalculate_local = true;
			}

			void set_scale(OEMaths::vec3f& scale)
			{
				local_trs.scale = scale;
				recalculate_local = true;
			}

			OEMaths::mat4f get_local_matrix()
			{
				if (recalculate_local) {
					calculate_local();
					recalculate_local = false;
				}
				return local;
			}

			void set_local_matrix(OEMaths::mat4f& local)
			{
				this->local = local;
			}

			void set_world_matrix(OEMaths::mat4f& world)
			{
				this->world = world;
			}

			void set_skin_index(const int32_t index)
			{
				skin_index = index;
			}

			int32_t get_skin_index() const
			{
				return skin_index;
			}

		private:

			bool recalculate_local = false;

			// decomposed form
			LocalTRS local_trs;

			OEMaths::mat4f local;
			OEMaths::mat4f world;

			// an index to skinning data for this particular node - negative number indicates no skin info
			int32_t skin_index = -1;

			// buffer offsets for tranform and skinned data
			uint32_t transform_buffer_offset = 0;
			uint32_t skinned_buffer_offset = 0;
		};

		// data that will be hosted on the gpu side
		struct TransformBufferInfo
		{
			OEMaths::mat4f model_matrix;
		};

		struct SkinnedBufferInfo
		{
			OEMaths::mat4f joint_matrices[6];
			float joint_count;
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
		const uint32_t TransformBlockSize = 25;
		const uint32_t SkinnedBlockSize = 25;

		TransformManager();
		~TransformManager();

		// update per frame 
		void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface) override;

		// gltf loading - The skinning data is going in the transform manager for now as it's needed most here for calculating skinning transforms
		void addGltfSkin(tinygltf::Model& model, std::unordered_map<uint32_t, Object>& linearised_objects);
		void addGltfTransform(tinygltf::Node& node, Object* obj, OEMaths::mat4f world_transform);

		// local transform and skinning update
		OEMaths::mat4f create_matrix(Object& obj, std::unique_ptr<ObjectManager>& obj_manager);
		void update_transform(std::unique_ptr<ObjectManager>& obj_manager);
		void update_transform_recursive(std::unique_ptr<ObjectManager>& obj_manager, Object& obj, uint32_t alignment, uint32_t skinned_alignment);

		// object update functions
		void update_obj_translation(Object& obj, OEMaths::vec4f trans);
		void update_obj_scale(Object& obj, OEMaths::vec4f scale);
		void update_obj_rotation(Object& obj, OEMaths::quatf rot);

		uint32_t get_transform_offset(uint32_t id)
		{
			// the transform buffer stores both parents and children so unfortunately we need to iterate through
			// to find the id.
			if (transformBuffer.find(id) == transformBuffer.end()) {
				LOGGER_ERROR("Unable to find object data with an id of %i within transform manager.", id);
			}
			return transformBuffer[id].get_transform_offset();
		}

		uint32_t get_skinned_offset(uint32_t id)
		{
			// the transform buffer stores both parents and children so unfortunately we need to iterate through
			// to find the id.
			if (transformBuffer.find(id) == transformBuffer.end()) {
				LOGGER_ERROR("Unable to find object data with an id of %i within transform manager.", id);
			}
			return transformBuffer[id].get_skinned_offset();
		}

	private:

		// transform data for static meshes - stored with object id for faster look up
		std::unordered_map<uint32_t, TransformData> transformBuffer;

		// skinned transform data
		std::vector<SkinInfo> skinBuffer;

		// store locally the aligned buffer sizes
		size_t transform_aligned = 0;
		size_t skinned_aligned = 0;

		// transform data for each object which will be added to the GPU
		TransformBufferInfo* transform_buffer_data = nullptr;
		SkinnedBufferInfo* skinned_buffer_data = nullptr;

		uint32_t transform_buffer_size = 0;
		uint32_t skinned_buffer_size = 0;

		// flag which tells us whether we need to update the static data
		bool is_dirty = true;
	};

}

