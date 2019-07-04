#include "OEModels.h"
#include "Engine/Omega_Global.h"
#include "Managers/EventManager.h"
#include "VulkanAPI/BufferManager.h"

namespace OmegaEngine
{
namespace Models
{

Cube::Cube()
{
	// vertex data
	VulkanAPI::BufferUpdateEvent vertexEvent{ "CubeModelVertices", vertices.data(),
		                                      vertices.size() * sizeof(float),
		                                      VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
	Global::eventManager()->instantNotification<VulkanAPI::BufferUpdateEvent>(vertexEvent);

	// index data
	VulkanAPI::BufferUpdateEvent indexEvent{ "CubeModelIndices", indices.data(),
		                                     indices.size() * sizeof(uint32_t),
		                                     VulkanAPI::MemoryUsage::VK_BUFFER_STATIC };
	Global::eventManager()->instantNotification<VulkanAPI::BufferUpdateEvent>(indexEvent);
}

Cube::~Cube()
{
}

Plane::Plane(const uint32_t patchSize, const float uvFactor)
{
	const float widthX = 3.0f;
	const float widthY = 3.0f;

	const uint32_t vertCount = patchSize * patchSize;
	mesh.vertices.reserve(vertCount);

	for (uint32_t x = 0; x < patchSize; ++x)
	{
		for (uint32_t y = 0; y < patchSize; ++y)
		{
			PlaneMesh::Vertex vertex;

			uint32_t index = x + y * patchSize;
			vertex.pos = OEMaths::vec3f{ x * widthX + widthX / 2.0f - (float)patchSize * widthX / 2.0f,
				0.0f, y * widthY + widthY / 2.0f - (float)patchSize * widthY / 2.0f };
			vertex.uv = OEMaths::vec2f{ x / (float)patchSize, y / (float)patchSize } * uvFactor;

			mesh.vertices.emplace_back(vertex);
		}
	}

	// generate the indices for the patch quads
	uint32_t width = patchSize - 1;
	uint32_t size = width * width * 4;

	mesh.indices.resize(size);

	for (uint32_t x = 0; x < width; ++x)
	{
		for (uint32_t y = 0; y < width; ++y)
		{
			uint32_t index = (x + y * width) * 4;
			mesh.indices[index] = (x + y * patchSize); // top-left
			mesh.indices[index + 1] = mesh.indices[index] + patchSize; // bottom-left
			mesh.indices[index + 2] = mesh.indices[index + 1] + 1; //	bottom-right
			mesh.indices[index + 3] = mesh.indices[index] + 1; // top-right
		}
	}
}

Sphere::Sphere(const uint32_t density)
{
	mesh.positions.reserve(6 * density * density);
	mesh.uvs.reserve(6 * density * density);
	mesh.indices.reserve(2 * density * density * 6);

	static const OEMaths::vec3f basePosition[6] = {
		OEMaths::vec3f(1.0f, 1.0f, 1.0f), OEMaths::vec3f(-1.0f, 1.0f, -1.0f),
		OEMaths::vec3f(-1.0f, 1.0f, -1.0f), OEMaths::vec3f(-1.0f, -1.0f, +1.0f),
		OEMaths::vec3f(-1.0f, 1.0f, +1.0f), OEMaths::vec3f(+1.0f, 1.0f, -1.0f),
	};

	static const OEMaths::vec3f dx[6] = {
		OEMaths::vec3f(0.0f, 0.0f, -2.0f),
		OEMaths::vec3f(0.0f, 0.0f, +2.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(-2.0f, 0.0f, 0.0f),
	};

	static const OEMaths::vec3f dy[6] = {
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
		OEMaths::vec3f(0.0f, 0.0f, +2.0f), OEMaths::vec3f(0.0f, 0.0f, -2.0f),
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
	};

	const float densityMod = 1.0f / static_cast<float>(density - 1);

	for (uint32_t face = 0; face < 6; face++)
	{
		uint32_t indexOffset = face * density * density;

		for (uint32_t y = 0; y < density; y++)
		{
			for (uint32_t x = 0; x < density; x++)
			{
				OEMaths::vec2f uv = OEMaths::vec2f{densityMod * x, densityMod * y};
				OEMaths::vec3f pos = basePosition[face] + dx[face] * uv.getX() + dy[face] * uv.getY();
				pos.normalise();
				mesh.positions.emplace_back(pos);
				mesh.uvs.emplace_back(uv);
			}
		}

		uint32_t strips = density - 1;
		for (uint32_t y = 0; y < strips; y++)
		{
			uint32_t baseIndex = indexOffset + y * density;
			for (unsigned x = 0; x < density; x++)
			{
				mesh.indices.emplace_back(baseIndex + x);
				mesh.indices.emplace_back(baseIndex + x + density);
			}
		}
	}
}

} // namespace Models
} // namespace OmegaEngine