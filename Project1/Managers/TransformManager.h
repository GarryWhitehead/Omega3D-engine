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

			OEMaths::mat4f& get_local()
			{
				return OEMaths::translate(OEMaths::mat4f, local_decomposed.trans) * local_decomposed.rot * OEMaths::scale(OEMaths::mat4f, local_decomposed.scale) * local;
			}

			OEMaths::mat4f& get_transform()
			{
				return transform;
			}

		private:

			struct LocalDecomposition
			{
				OEMaths::vec3f trans;
				OEMaths::vec3f scale;
				OEMaths::mat4f rot;

			} local_decomposed;

			OEMaths::mat4f local;
			OEMaths::mat4f world;
			OEMaths::mat4f transform;
		};

		TransformManager();
		~TransformManager();

		uint32_t addGltfTransform(tinygltf::Node& node, Object& obj, OEMaths::mat4f world_transform);

		void generate_static_transform();

		OEMaths::mat4f& get_transform(Object obj)
		{
			if (objects.find(obj) == objects.end()) {
				// just log a warning here that we are looking for something that doesn't exsist (obviously bad!)
				LOGGER_INFO("Unable to find object with id: %i\n", obj.get_id());
			}
			return transformBuffer[objects[obj]].get_transform();
		}

	private:

		// a list of all objects associated with this manager and their position within the main data buffer
		std::unordered_map<Object, uint32_t, HashObject> objects;

		std::vector<TransformData> transformBuffer;
	};

}

