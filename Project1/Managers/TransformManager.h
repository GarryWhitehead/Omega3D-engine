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

		class TransformData
		{

		public:

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

			OEMaths::mat4f& get_transform()
			{
				return transform;
			}

			void set_transform(const OEMaths::mat4f& trans)
			{
				transform = trans;
			}

			void set_world(const OEMaths::mat4f& mat)
			{
				world = mat;
			}

			void set_trs(const LocalTRS& trs)
			{
				local_trs = trs;
			}

			void set_local(const OEMaths::mat4f& mat)
			{
				local = mat;
			}

		private:

			LocalTRS local_trs;

			OEMaths::mat4f local;
			OEMaths::mat4f world;
			OEMaths::mat4f transform;
		};

		// skins
		struct SkinInfo
		{
			const char* name;
			uint32_t skeletonIndex;

			std::vector<Object> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
			std::vector<OEMaths::mat4f> joint_matrices;
		};

		TransformManager();
		~TransformManager();

		void update_frame(double time, double dt) override;

		// gltf loading - The skinning data is going in the transform manager for now as is needed most here for calculating skinning transforms
		void addGltfSkin(tinygltf::Model& model);
		uint32_t addGltfTransform(tinygltf::Node& node, Object& obj, OEMaths::mat4f world_transform);

		// static update
		void update_static(std::unique_ptr<ObjectManager>& obj_manager);
		void update_static_recursive(uint32_t index, Object& obj);

		// skinned update
		void update_skinned(std::unique_ptr<ObjectManager>& obj_manager);
		void update_skinned_recursive(std::unique_ptr<ObjectManager>& obj_manager, uint32_t anim_index, Object& obj);

		OEMaths::mat4f& get_transform(uint32_t transform_index)
		{
			assert(transform_index < transformBuffer.size());
			return transformBuffer[transform_index].get_transform;
		}

	private:

		// transform data for static meshes
		std::vector<TransformData> transformBuffer;

		// skinned transform data
		std::vector<SkinInfo> skinBuffer;

		// flag which tells us whether we need to update the static data
		bool is_static_dirty = true;
		bool is_skinned_dirty = true;
	};

}

