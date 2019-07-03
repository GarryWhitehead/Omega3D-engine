#pragma once

#include "OEMaths/OEMaths.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/MemoryAllocator.h"

#include <array>
#include <cstdint>

namespace OmegaEngine
{
namespace RenderUtil
{

class CubeModel
{
public:
	static constexpr uint32_t indicesSize = 36;
	static constexpr uint32_t verticesSize = 24;

	// cube vertices
	std::array<float, verticesSize> vertices{
		// front
		-1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
		// back
		-1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f
	};

	// cube indices
	std::array<uint32_t, indicesSize> indices{ // front
		                                       0, 1, 2, 2, 3, 0,
		                                       // right side
		                                       1, 5, 6, 6, 2, 1,
		                                       // back
		                                       7, 6, 5, 5, 4, 7,
		                                       // left side
		                                       4, 0, 3, 3, 7, 4,
		                                       // bottom
		                                       4, 5, 1, 1, 0, 4,
		                                       // top
		                                       3, 2, 6, 6, 7, 3
	};

	CubeModel();
	~CubeModel();
};

class PlaneModel
{
public:
	struct PlaneMesh
	{
		struct Vertex
		{
			OEMaths::vec2f uv;
			OEMaths::vec3f pos;
			OEMaths::vec3f normal;
		};

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
	};

	PlaneModel(const uint32_t patchSize, const float uvFactor);

	// TODO: destructor should remove buffers from buffer manager

	PlaneMesh mesh;
};

class SphereModel
{
public:
	struct SphereMesh
	{
		std::vector<OEMaths::vec3f> positions;
		std::vector<OEMaths::vec2f> uvs;
		std::vector<uint32_t> indices;
	};

	SphereModel(const uint32_t density);

private:
	SphereMesh mesh;
};
} // namespace RenderUtil
} // namespace OmegaEngine