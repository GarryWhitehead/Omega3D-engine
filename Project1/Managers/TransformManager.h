#pragma once
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "ComponentInterface/ComponentManagerBase.h"
#include "Utility/logger.h"
#include "tiny_gltf.h"

namespace OmegaEngine
{
	// forward decleartions
	class Object;
	class ObjectManager;

	class TransformManager : public ComponentManagerBase
	{

	public:

		struct HashObject
		{
			size_t operator()(const Object& obj) const
			{
				return(std::hash<uint32_t>()(obj.get_id()));
			}
		};

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

		TransformManager();
		~TransformManager();

		void update_frame(double time, double dt) override;

		// gltf loading - The skinning data is going in the transform manager for now as is needed most here for calculating skinning transforms
		void addGltfSkin(tinygltf::Model& model, std::vector<Object>& linearised_objects);
		uint32_t addGltfTransform(tinygltf::Node& node, Object& obj, OEMaths::mat4f world_transform);

		void update_transform(std::unique_ptr<ObjectManager>& obj_manager);
		void update_transform_recursive(uint32_t index, Object& obj);

		OEMaths::mat4f& get_transform(uint32_t transform_index)
		{
			assert(transform_index < transformBuffer.size());
			return transformBuffer[transform_index].transform;
		}

	private:

		// transform data for static meshes
		std::vector<TransformData> transformBuffer;

		// skinned transform data
		std::vector<SkinInfo> skinBuffer;

		// transform data for each object which will be added to the GPU
		std::vector<TransformBufferInfo> transform_buffer_info;
		std::vector<SkinnedBufferInfo> skinned_buffer_info;

		// flag which tells us whether we need to update the static data
		bool is_dirty = true;
	};

}

