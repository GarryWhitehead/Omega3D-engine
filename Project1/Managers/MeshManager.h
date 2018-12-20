#pragma once

#include "tiny_gltf.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include <memory>
#include <tuple>

namespace OmegaEngine
{

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
			OEMaths::vec4f joint;
			OEMaths::vec4f weight;
		};

		
		struct PrimitiveInfo
		{
			PrimitiveInfo(uint32_t offset, uint32_t size, uint32_t matid, OEMaths::vec3f min, OEMaths::vec3f max) :
				indiciesOffset(offset),
				indiciesSize(size),
				materialId(matid)
			{
				dimensions.initDimensions(min, max);
			}
			
			Dimensions dimensions;
			
			// index offsets
			uint32_t indiciesOffset;
			uint32_t indiciesSize;

			// material id
			uint32_t materialId;
		};


		struct StaticMesh
		{
			Dimensions dimensions;

			// vertex offsets
			uint32_t vertexOffset;
			uint32_t vertexSize;

			// primitives assoicated with this mesh
			std::vector<PrimitiveInfo> primitives;

		};

		struct Node
		{
			const char* name;
			uint32_t index;

			// rather than using pointers and lots of buffers, to improve cacahe performance, nodes are all stored in one large buffer and referenced via indicies into the buffer
			uint32_t parentIndex = -1;
			std::vector<uint32_t> children;

			std::tuple<uint32_t, uint32_t> meshIndex;

			// transforms associated with this node
			OEMaths::mat4f matrix;
			OEMaths::vec3f translation;
			OEMaths::vec3f scale{ 1.0f };
			OEMaths::mat4f rotation;

			// skinning info
			uint32_t skinIndex;

		};

		MeshManager();
		~MeshManager();

		void parseGltfFile(uint32_t spaceId, tinygltf::Model& model);
		void parseNode(uint32_t parentNode, 
			tinygltf::Node& node, 
			tinygltf::Model& model,
			std::vector<Vertex>& vertexBuffer, 
			std::vector<uint32_t>& indexBuffer, 
			std::vector<StaticMesh>& meshBuffer, 
			std::vector<Node>& nodeBuffer,
			std::vector<Node>& linearBuffer, 
			uint32_t spaceId);

		template <typename T>
		void parseIndices(tinygltf::Accessor accessor, tinygltf::BufferView bufferView, tinygltf::Buffer buffer, std::vector<T>& indiciesBuffer, uint32_t indexStart)
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

		// the mega buffers containing all the model data. The number of spaces that will be loaded at once depends on the specs of the system 
		std::unordered_map<uint32_t, std::vector<Vertex> > vertexBuffer;
		std::unordered_map<uint32_t, std::vector<uint32_t> > indexBuffer;
		std::unordered_map<uint32_t, std::vector<StaticMesh> > meshBuffer;

		std::unordered_map<uint32_t, std::vector<Node> > nodeBuffer;
		std::unordered_map<uint32_t, std::vector<Node> > linearNodeBuffer;
	};

}

