#pragma once

#include "OEMaths/OEMaths.h"

#include "tiny_gltf.h"

namespace OmegaEngine
{

	class ModelMesh
	{

	public:

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
			OEMaths::vec4f weight;
			OEMaths::vec4f joint;
		};

		struct Primitive
		{
			Primitive(uint32_t offset, uint32_t size, OEMaths::vec3f min, OEMaths::vec3f max) :
				indexBase(offset),
				indexCount(size)
			{
				// add to bvh here?
			}

			Dimensions dimensions;

			// index offsets
			uint32_t indexBase = 0;
			uint32_t indexCount = 0;
		};

		ModelMesh();
		~ModelMesh();

		void ModelMesh::extractMeshData(tinygltf::Model& model, tinygltf::Node& node);

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

		Dimensions totalDimensions;

		std::vector<Primitive> primitives;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		bool hasSkin = false;
	};

}

