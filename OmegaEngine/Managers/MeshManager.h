#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "Managers/ManagerBase.h"
#include "Utility/logger.h"

#include "tiny_gltf.h"

#include <memory>
#include <tuple>
#include <unordered_map>


namespace OmegaEngine
{
	// forard decleartions
	class ObjectManager;
	class Object;

	class MeshManager : public ManagerBase
	{

	public:
		
		// a user-defined size for the vertex and index gpu mem blocks - this should maybe made more dynamic? Also needs checking for overspill....
		static constexpr float VertexBlockSize = 1e+5;
		static constexpr float IndexBlockSize = 1e+5;

		enum class MeshType
		{
			Static,
			Skinned
		};

		struct Dimensions
		{
			OEMaths::vec3f min;
			OEMaths::vec3f max;
			OEMaths::vec3f size;
			OEMaths::vec3f center;
			float radius;

			//void initDimensions(OEMaths::vec3f min, OEMaths::vec3f max);
		};

		struct Vertex
		{
			OEMaths::vec4f position;
			OEMaths::vec2f uv0;
			OEMaths::vec2f uv1;
			OEMaths::vec3f normal;
		};

		struct SkinnedVertex 
		{
			OEMaths::vec4f position;
			OEMaths::vec2f uv0;
			OEMaths::vec2f uv1;
			OEMaths::vec3f normal;
			OEMaths::vec4f weight;
			OEMaths::vec4f joint;
		};

		struct PrimitiveMesh
		{
			PrimitiveMesh(MeshType _type, uint32_t offset, uint32_t size, uint32_t matid, OEMaths::vec3f min, OEMaths::vec3f max) :
				type(_type),
				indexBase(offset),
				indexCount(size),
				materialId(matid)
			{
				// add to bvh here?
			}

			MeshType type;
			Dimensions dimensions;

			// index offsets
			uint32_t indexBase = 0;
			uint32_t indexCount = 0;
			
			// material id
			uint32_t materialId;
		};

		struct StaticMesh
		{
			MeshType type;
			Dimensions dimensions;

			// primitives assoicated with this mesh
			std::vector<PrimitiveMesh> primitives;

			// offset into mega buffer
			uint32_t vertexBufferOffset;
			uint32_t indexBufferOffset;
		};

		MeshManager();
		~MeshManager();

		// on a per-frame basis - if the mesh data is dirty then deal with that here (e.g. transforms to meshes, deletion, removal from gpu side...) 
		void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface) override;

		void addGltfMesh(tinygltf::Model& model, tinygltf::Node& node, Object* obj, uint32_t& globalVertexCount, uint32_t& globalIndexCount);

		StaticMesh& getMesh(MeshComponent comp)
		{
			assert(comp.index < meshBuffer.size());
			return meshBuffer[comp.index];
		}

		template <typename T>
		void parseIndices(tinygltf::Accessor accessor, tinygltf::BufferView bufferView, tinygltf::Buffer buffer, std::vector<uint32_t>& indiciesBuffer, uint32_t indexStart)
		{
			T* buf = new T[accessor.count];
			memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(T));

			// copy the data to our indices buffer at the correct offset
			for (uint32_t j = 0; j < accessor.count; ++j) 
			{
				indiciesBuffer.push_back(buf[j] + indexStart);
			}

			delete buf;
		}

	private:

		// the buffers containing all the model data 
		std::vector<StaticMesh> meshBuffer;

		// all vertices and indices held in one large buffer
		std::vector<Vertex> static_vertices;
		std::vector<SkinnedVertex> skinnedVertices;
		std::vector<uint32_t> indices;

		bool isDirty = true;
	};

}

