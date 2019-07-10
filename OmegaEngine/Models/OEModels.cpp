#include "OEModels.h"

#include <array>

namespace OmegaEngine
{
namespace OEModels
{

std::unique_ptr<OmegaEngine::ModelMesh>& generatePlaneMesh(const uint32_t size, const uint32_t uvFactor)
{
	const float widthX = 3.0f;
	const float widthY = 3.0f;

	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	const uint32_t vertCount = size * size;
	mesh->vertices.reserve(vertCount);

	for (uint32_t x = 0; x < size; ++x)
	{
		for (uint32_t y = 0; y < size; ++y)
		{
			OmegaEngine::ModelMesh::Vertex vertex;

			uint32_t index = x + y * size;
			vertex.position = OEMaths::vec4f{
				x * widthX + widthX / 2.0f - static_cast<float>(size) * widthX / 2.0f, 0.0f,
				y * widthY + widthY / 2.0f - static_cast<float>(size) * widthY / 2.0f, 1.0f
			};
			vertex.uv0 =
			    OEMaths::vec2f{ x / static_cast<float>(size), y / static_cast<float>(size) } *
			    uvFactor;

			mesh->vertices.emplace_back(vertex);
		}
	}

	// generate the indices for the patch quads
	uint32_t width = size - 1;
	uint32_t indicesSize = width * width * 4;

	mesh->indices.resize(indicesSize);

	for (uint32_t x = 0; x < width; ++x)
	{
		for (uint32_t y = 0; y < width; ++y)
		{
			uint32_t index = (x + y * width) * 4;
			mesh->indices[index] = (x + y * size); // top-left
			mesh->indices[index + 1] = mesh->indices[index] + size; // bottom-left
			mesh->indices[index + 2] = mesh->indices[index + 1] + 1; //	bottom-right
			mesh->indices[index + 3] = mesh->indices[index] + 1; // top-right
		}
	}

	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });

	return std::move(mesh);
}

std::unique_ptr<OmegaEngine::ModelMesh> &generateSphereMesh(const uint32_t density)
{
	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	mesh->vertices.reserve(6 * density * density);
	mesh->indices.reserve(2 * density * density * 6);

	static const OEMaths::vec3f basePosition[6] = {
		OEMaths::vec3f(1.0f, 1.0f, 1.0f),   OEMaths::vec3f(-1.0f, 1.0f, -1.0f),
		OEMaths::vec3f(-1.0f, 1.0f, -1.0f), OEMaths::vec3f(-1.0f, -1.0f, +1.0f),
		OEMaths::vec3f(-1.0f, 1.0f, +1.0f), OEMaths::vec3f(+1.0f, 1.0f, -1.0f),
	};

	static const OEMaths::vec3f dx[6] = {
		OEMaths::vec3f(0.0f, 0.0f, -2.0f), OEMaths::vec3f(0.0f, 0.0f, +2.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(2.0f, 0.0f, 0.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(-2.0f, 0.0f, 0.0f),
	};

	static const OEMaths::vec3f dy[6] = {
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
		OEMaths::vec3f(0.0f, 0.0f, +2.0f), OEMaths::vec3f(0.0f, 0.0f, -2.0f),
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
	};

	const float densityMod = 1.0f / static_cast<float>(density - 1);

	for (uint32_t face = 0; face < 6; ++face)
	{
		uint32_t indexOffset = face * density * density;

		for (uint32_t y = 0; y < density; ++y)
		{
			for (uint32_t x = 0; x < density; ++x)
			{
				OmegaEngine::ModelMesh::Vertex vertex;
				vertex.uv0 = OEMaths::vec2f{ densityMod * x, densityMod * y };
				vertex.position =
				    OEMaths::vec4f{ basePosition[face] + dx[face] * vertex.uv0.getX() +
					                    dy[face] * vertex.uv0.getY(),
					                1.0f };
				vertex.position.normalise();
				mesh->vertices.emplace_back(vertex);
			}
		}

		uint32_t strips = density - 1;
		for (uint32_t y = 0; y < strips; ++y)
		{
			uint32_t baseIndex = indexOffset + y * density;
			for (uint32_t x = 0; x < density; ++x)
			{
				mesh->indices.emplace_back(baseIndex + x);
				mesh->indices.emplace_back(baseIndex + x + density);
			}
		}
	}

	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });

	return std::move(mesh);
}

std::unique_ptr<OmegaEngine::ModelMesh> &generateCubeMesh(const OEMaths::vec3f &size)
{
	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	const float x = size.getX() / 2.0f;
	const float y = size.getY() / 2.0f;
	const float z = size.getZ() / 2.0f;

	// cube vertices
	static const std::array<OEMaths::vec3f, 8> vertexData{
		OEMaths::vec3f{ +x, +y, +z }, OEMaths::vec3f{ -x, +y, +z }, OEMaths::vec3f{ -x, -y, +z },
		OEMaths::vec3f{ +x, -y, +z }, OEMaths::vec3f{ +x, +y, -z }, OEMaths::vec3f{ -x, +y, -z },
		OEMaths::vec3f{ -x, -y, -z }, OEMaths::vec3f{ +x, -y, -z }
	};

	// cube uv
	static const std::array<OEMaths::vec2f, 8> uvData{
		OEMaths::vec2f{ 1.0f, 1.0f }, OEMaths::vec2f{ 0.0f, 1.0f }, OEMaths::vec2f{ 0.0f, 0.0f },
		OEMaths::vec2f{ 1.0f, 0.0f }, OEMaths::vec2f{ 0.0f, 1.0f }, OEMaths::vec2f{ 1.0f, 1.0f },
		OEMaths::vec2f{ 0.0f, 0.0f }, OEMaths::vec2f{ 1.0f, 0.0f }
	};

	// cube indices
	static const std::array<uint32_t, 36> indexData{ // front
		                                             1, 2, 3, 3, 0, 1,
		                                             // right side
		                                             2, 6, 7, 7, 3, 2,
		                                             // back
		                                             6, 5, 4, 4, 7, 6,
		                                             // left side
		                                             5, 1, 0, 0, 4, 5,
		                                             // bottom
		                                             0, 3, 7, 7, 4, 0,
		                                             // top
		                                             5, 6, 2, 2, 1, 5
	};

	// generate normals
	/*for (int i = 0; i < 36; i += 3)
	{
		glm::vec3 normal =
		    glm::normalize(glm::cross(glm::vec3(verts[i + 1]) - glm::vec3(verts[i]),
		                              glm::vec3(verts[i + 2]) - glm::vec3(verts[i])));

		norm[i] = normal;
		norm[i + 1] = normal;
		norm[i + 2] = normal;
	}*/

	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		OmegaEngine::ModelMesh::Vertex vertex;
		vertex.position = OEMaths::vec4f{ vertexData[i], 1.0f };
		vertex.uv0 = uvData[i];
		mesh->vertices.emplace_back(vertex);
	}

	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });

	return std::move(mesh);
}

} // namespace Models
} // namespace OmegaEngine