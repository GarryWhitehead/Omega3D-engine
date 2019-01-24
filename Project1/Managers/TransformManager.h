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

		TransformManager();
		~TransformManager();

		uint32_t addGltfTransform(tinygltf::Node& node, Object& obj, OEMaths::mat4f world_transform);

		void update_static(std::unique_ptr<ObjectManager>& obj_manager);
		void update_static_recursive(uint32_t index, Object& obj);

		OEMaths::mat4f& get_transform(uint32_t transform_index)
		{
			assert(transform_index < transBuffer.size());
			return transformBuffer[transform_index].get_transform;
		}

	private:

		std::vector<TransformData> transformBuffer;
	};

}

