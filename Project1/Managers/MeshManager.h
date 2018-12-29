#pragma once

#include "tiny_gltf.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "Engine/GltfParser.h"

#include <memory>
#include <tuple>
#include <unordered_map>


namespace OmegaEngine
{
	// forard decleartions
	struct GltfVertex;
	struct GltfStaticMeshInfo;
	struct GltfNodeInfo;
	class ObjectManager;
	class MaterialManager;
	class Object;

	class MeshManager
	{

	public:
		
		struct Dimensions
		{
			OEMaths::vec3f min;
			OEMaths::vec3f max;
			OEMaths::vec3f size;
			OEMaths::vec3f center;
			float radius;

			void initDimensions(OEMaths::vec3f min, OEMaths::vec3f max);
		};

		struct Vertex
		{
			OEMaths::vec4f position;
			OEMaths::vec3f normal;
			OEMaths::vec2f uv;
			OEMaths::vec4f weight;
			OEMaths::vec4f joint;
		};

		struct PrimitiveMesh
		{
			PrimitiveMesh(uint32_t offset, uint32_t size, uint32_t matid, OEMaths::vec3f min, OEMaths::vec3f max) :
				indexBase(offset),
				indexCount(size),
				materialId(matid)
			{
				dimensions.initDimensions(min, max);
			}

			Dimensions dimensions;

			// index offsets
			uint32_t indexBase = 0;
			uint32_t indexCount = 0;
			

			// material id
			uint32_t materialId;
		};

		struct StaticMesh
		{
			Dimensions dimensions;

			uint32_t verticesOffset = 0;
			uint32_t indicesOffset = 0;

			// primitives assoicated with this mesh
			std::vector<PrimitiveMesh> primitives;

		};


		MeshManager();
		~MeshManager();

		void addGltfData(tinygltf::Model& model, tinygltf::Node& node, Object& obj);

		template <typename T>
		void parseIndices(tinygltf::Accessor accessor, tinygltf::BufferView bufferView, tinygltf::Buffer buffer, std::vector<uint32_t>& indiciesBuffer, uint32_t indexStart)
		{
			T* buf = new T[accessor.count];
			memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(T));

			// copy the data to our indices buffer at the correct offset
			for (uint32_t j = 0; j < indAccessor.count; ++j) {
				indiciesBuffer.push_back(buf[j] + indexStart);
			}

			delete buf;
		}

	private:

		// the buffers containing all the model data 
		std::vector<StaticMesh> staticMeshBuffer;

		// all vertex and index data is stored in one long continous buffer accessed by offsets for each space
		std::vector<Vertex> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		std::unique_ptr<MaterialManager> materialManager;
	};

}

