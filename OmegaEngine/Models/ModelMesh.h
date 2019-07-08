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
		Primitive(uint32_t offset, uint32_t size, int32_t matId)
		    : indexBase(offset)
		    , indexCount(size)
		    , materialId(matId)
		{
			// add to bvh here?
		}

		Dimensions dimensions;
		int32_t materialId = -1;

		// index offsets
		uint32_t indexBase = 0;
		uint32_t indexCount = 0;
	};

	ModelMesh();
	~ModelMesh();

	void generatePlaneMesh(const uint32_t size, const uint32_t uvFactor);
	void generateSphereMesh(const uint32_t density);
	void generateCubeMesh(const OEMaths::vec3f &size);

	void extractGltfMeshData(tinygltf::Model &model, tinygltf::Node &node);

	template <typename T>
	void parseIndices(tinygltf::Accessor accessor, tinygltf::BufferView bufferView,
	                  tinygltf::Buffer buffer, std::vector<uint32_t> &indiciesBuffer,
	                  uint32_t indexStart)
	{
		T *buf = new T[accessor.count];
		memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset],
		       accessor.count * sizeof(T));

		// copy the data to our indices buffer at the correct offset
		for (uint32_t j = 0; j < accessor.count; ++j)
		{
			indiciesBuffer.push_back(buf[j] + indexStart);
		}

		delete buf;
	}

	std::vector<Vertex> &getVertices()
	{
		return vertices;
	}

	std::vector<uint32_t> &getIndices()
	{
		return indices;
	}

	bool hasSkin() const
	{
		return skinned;
	}

	std::vector<Primitive> &getPrimitives()
	{
		return primitives;
	}

private:
	Dimensions totalDimensions;

	std::vector<Primitive> primitives;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	bool skinned = false;
};

} // namespace OmegaEngine
