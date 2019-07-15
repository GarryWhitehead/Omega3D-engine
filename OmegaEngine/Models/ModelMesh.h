#pragma once

#include "OEMaths/OEMaths.h"

#include <vector>

namespace OmegaEngine
{

// forward declerations
enum class StateTopology;

struct ModelMesh
{
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

	// defines the topology to use in the program state
	StateTopology topology;

	Dimensions totalDimensions;

	std::vector<Primitive> primitives;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	bool skinned = false;
};

} // namespace OmegaEngine
