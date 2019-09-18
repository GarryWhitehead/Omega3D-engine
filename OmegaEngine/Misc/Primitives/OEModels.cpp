#include "OEModels.h"
#include "Rendering/ProgramStateManager.h"

#include <array>

namespace OmegaEngine
{
namespace OEModels
{

std::unique_ptr<OmegaEngine::ModelMesh> generateQuadMesh(const float size)
{
	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	mesh->vertices.resize(4);
	mesh->vertices[0].position = OEMaths::vec4f{ size, size, 0.0f, 1.0f };
	mesh->vertices[1].position = OEMaths::vec4f{ -size, size, 0.0f, 1.0f };
	mesh->vertices[2].position = OEMaths::vec4f{ -size, -size, 0.0f, 1.0f };
	mesh->vertices[3].position = OEMaths::vec4f{ size, -size, 0.0f, 1.0f };

	mesh->vertices[0].uv0 = OEMaths::vec2f{ 1.0f, 1.0f };
	mesh->vertices[1].uv0 = OEMaths::vec2f{ 0.0f, 1.0f };
	mesh->vertices[2].uv0 = OEMaths::vec2f{ 0.0f, 0.0f };
	mesh->vertices[3].uv0 = OEMaths::vec2f{ 1.0f, 0.0f };

	mesh->vertices[0].normal = OEMaths::vec3f{ 0.0f, 0.0f, -1.0f };
	mesh->vertices[1].normal = OEMaths::vec3f{ 0.0f, 0.0f, -1.0f };
	mesh->vertices[2].normal = OEMaths::vec3f{ 0.0f, 0.0f, -1.0f };
	mesh->vertices[3].normal = OEMaths::vec3f{ 0.0f, 0.0f, -1.0f };

	// quad made up of two triangles
	mesh->indices.resize(6);
	std::array<uint32_t, 6> indices = { 0, 1, 2, 2, 3, 0 };
	for (uint32_t i = 0; i < 6; ++i)
	{ 
		mesh->indices[i] = indices[i];
	}

	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });
	mesh->topology = StateTopology::List;

	return std::move(mesh);
}   

std::unique_ptr<OmegaEngine::ModelMesh> generateSphereMesh(const uint32_t density)
{
	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	mesh->vertices.reserve(6 * density * density);
	mesh->indices.reserve(2 * density * density * 6);

	static const OEMaths::vec3f basePosition[6] = {
		OEMaths::vec3f(1.0f, 1.0f, 1.0f),    OEMaths::vec3f(-1.0f, 1.0f, -1.0f), OEMaths::vec3f(-1.0f, 1.0f, -1.0f),
		OEMaths::vec3f(-1.0f, -1.0f, +1.0f), OEMaths::vec3f(-1.0f, 1.0f, +1.0f), OEMaths::vec3f(+1.0f, 1.0f, -1.0f),
	};

	static const OEMaths::vec3f dx[6] = {
		OEMaths::vec3f(0.0f, 0.0f, -2.0f), OEMaths::vec3f(0.0f, 0.0f, +2.0f), OEMaths::vec3f(2.0f, 0.0f, 0.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(-2.0f, 0.0f, 0.0f),
	};

	static const OEMaths::vec3f dy[6] = {
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, 0.0f, +2.0f),
		OEMaths::vec3f(0.0f, 0.0f, -2.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
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
				OEMaths::vec3f pos =
				    OEMaths::vec3f{ basePosition[face] + dx[face] * vertex.uv0.getX() + dy[face] * vertex.uv0.getY() };
				pos.normalise();
				vertex.position = OEMaths::vec4f{ pos, 1.0f };
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
			mesh->indices.emplace_back(UINT32_MAX);
		}
	}

	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });
	// using primitive rerstart here so when the 0xFFFF value is read, the most recent indices values
	// will be discarded
	mesh->topology = StateTopology::StripRestart;

	return std::move(mesh);
}

std::unique_ptr<OmegaEngine::ModelMesh> generateCapsuleMesh(const uint32_t density, const float height,
                                                            const float radius)
{
	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	const uint32_t innerSize = density / 2;
	const float halfHeight = 0.5f * height - 0.5f * radius;
	float invDensity = 1.0f / static_cast<float>(density);

	mesh->vertices.resize(2 * innerSize * density + 2);

	// Top center
	mesh->vertices[0].position = OEMaths::vec4f{ 0.0f, halfHeight + radius, 0.0f, 1.0f };
	mesh->vertices[0].normal = OEMaths::vec3f{ 0.0f, 1.0f, 0.0f };

	// Bottom center
	mesh->vertices[1].position = OEMaths::vec4f{ 0.0f, -halfHeight + radius, 0.0f, 1.0f };
	mesh->vertices[1].normal = OEMaths::vec3f{ 0.0f, -1.0f, 0.0f };

	// Top rings
	for (uint32_t i = 0; i < innerSize; ++i)
	{
		float w = float(i + 1) / static_cast<float>(innerSize);
		float extraHeight = radius * std::sqrtf(1.0f - w * w);
		uint32_t offset = i * density + 2;

		for (uint32_t j = 0; j < density; ++j)
		{
			float rad = 2.0f * static_cast<float>(M_PI) * (j + 0.5f) * invDensity;
			mesh->vertices[offset + j].position =
			    OEMaths::vec4f(w * radius * std::cos(rad), halfHeight + extraHeight, -w * radius * std::sin(rad), 1.0f);

			OEMaths::vec3f norm{ mesh->vertices[offset + j].position.getX(), extraHeight,
				                 mesh->vertices[offset + j].position.getZ() };
			norm.normalise();
			mesh->vertices[offset + j].normal = norm;
		}
	}

	// Bottom rings
	for (uint32_t i = 0; i < innerSize; ++i)
	{
		float w = static_cast<float>(innerSize - i) / static_cast<float>(innerSize);
		float extraHeight = radius * std::sqrt(1.0f - w * w);
		uint32_t offset = (i + innerSize) * density + 2;

		for (uint32_t j = 0; j < density; ++j)
		{
			float rad = 2.0f * static_cast<float>(M_PI) * (j + 0.5f) * invDensity;
			mesh->vertices[offset + j].position = OEMaths::vec4f(w * radius * std::cos(rad), -halfHeight - extraHeight,
			                                                     -w * radius * std::sin(rad), 1.0f);

			OEMaths::vec3f norm{ mesh->vertices[offset + j].position.getX(), -extraHeight,
				                 mesh->vertices[offset + j].position.getZ() };
			norm.normalise();
			mesh->vertices[offset + j].normal = norm;
		}
	}

	// Link up top vertices.
	for (uint32_t i = 0; i < density; ++i)
	{
		mesh->indices.emplace_back(0);
		mesh->indices.emplace_back(i + 2);
		mesh->indices.emplace_back(((i + 1) % density) + 2);
	}

	// Link up bottom vertices.
	for (uint32_t i = 0; i < density; i++)
	{
		mesh->indices.emplace_back(1);
		mesh->indices.emplace_back((2 * innerSize - 1) * density + ((i + 1) % density) + 2);
		mesh->indices.emplace_back((2 * innerSize - 1) * density + i + 2);
	}

	// Link up rings.
	for (uint32_t i = 0; i < 2 * innerSize - 1; ++i)
	{
		uint32_t offset0 = i * density + 2;
		uint32_t offset1 = offset0 + density;

		for (uint32_t j = 0; j < density; ++j)
		{
			mesh->indices.emplace_back(offset0 + j);
			mesh->indices.emplace_back(offset1 + j);
			mesh->indices.emplace_back(offset0 + ((j + 1) % density));
			mesh->indices.emplace_back(offset1 + ((j + 1) % density));
			mesh->indices.emplace_back(offset0 + ((j + 1) % density));
			mesh->indices.emplace_back(offset1 + j);
		}
	}

	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });
	mesh->topology = StateTopology::List;

	return std::move(mesh);
}

std::unique_ptr<OmegaEngine::ModelMesh> generateCubeMesh(const OEMaths::vec3f& size)
{
	auto mesh = std::make_unique<OmegaEngine::ModelMesh>();

	const float x = size.getX() / 2.0f;
	const float y = size.getY() / 2.0f;
	const float z = size.getZ() / 2.0f;

	// cube vertices
	const OEMaths::vec3f v0{ +x, +y, +z };
	const OEMaths::vec3f v1{ -x, +y, +z };
	const OEMaths::vec3f v2{ -x, -y, +z };
	const OEMaths::vec3f v3{ +x, -y, +z };
	const OEMaths::vec3f v4{ +x, +y, -z };
	const OEMaths::vec3f v5{ -x, +y, -z };
	const OEMaths::vec3f v6{ -x, -y, -z };
	const OEMaths::vec3f v7{ +x, -y, -z };

	// cube uvs
	const OEMaths::vec2f uv0{ 1.0f, 1.0f };
	const OEMaths::vec2f uv1{ 0.0f, 1.0f };
	const OEMaths::vec2f uv2{ 0.0f, 0.0f };
	const OEMaths::vec2f uv3{ 1.0f, 0.0f };
	const OEMaths::vec2f uv4{ 0.0f, 1.0f };
	const OEMaths::vec2f uv5{ 1.0f, 1.0f };
	const OEMaths::vec2f uv6{ 0.0f, 0.0f };
	const OEMaths::vec2f uv7{ 1.0f, 0.0f };


	static const std::array<OEMaths::vec3f, 36> vertexData = { v1, v2, v3, v3, v0, v1, v2, v6, v7, v7, v3, v2,
		                                                       v6, v5, v4, v4, v7, v6, v5, v1, v0, v0, v4, v5,
		                                                       v0, v3, v7, v7, v4, v0, v5, v6, v2, v2, v1, v5 };

	static const std::array<OEMaths::vec2f, 36> uvData = { uv1, uv2, uv3, uv3, uv0, uv1, uv2, uv6, uv7, uv7, uv3, uv2,
		                                                   uv6, uv5, uv4, uv4, uv7, uv6, uv5, uv1, uv0, uv0, uv4, uv5,
		                                                   uv0, uv3, uv7, uv7, uv4, uv0, uv5, uv6, uv2, uv2, uv1, uv5 };

	static const std::array<OEMaths::vec3f, 6> normalData = {
		OEMaths::vec3f{ 0.0f, 0.0f, +1.0f }, OEMaths::vec3f{ -1.0f, 0.0f, 0.0f }, OEMaths::vec3f{ 0.0f, 0.0f, -1.0f },
		OEMaths::vec3f{ +1.0f, 0.0f, 0.0f }, OEMaths::vec3f{ 0.0f, -1.0f, 0.0f }, OEMaths::vec3f{ 0.0f, +1.0f, 0.0f }
	};

	// cube indices
	static const std::array<uint32_t, 36> indexData{ // front
		                                             0, 1, 2, 3, 4, 5,
		                                             // right side
		                                             6, 7, 8, 9, 10, 11,
		                                             // back
		                                             12, 13, 14, 15, 16, 17,
		                                             // left side
		                                             18, 19, 20, 21, 22, 23,
		                                             // bottom
		                                             24, 25, 26, 27, 28, 29,
		                                             // top
		                                             30, 31, 32, 33, 34, 35
	};

	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		OmegaEngine::ModelMesh::Vertex vertex;
		vertex.position = OEMaths::vec4f{ vertexData[i], 1.0f };
		vertex.uv0 = uvData[i];
		mesh->vertices.emplace_back(vertex);
	}

	// sort the normal - per face
	uint32_t vertexBase = 0;
	for (uint32_t i = 0; i < 6; ++i)
	{
		for (uint32_t j = 0; j < 6; ++j)
		{
			mesh->vertices[vertexBase + j].normal = normalData[i];
		}
		vertexBase += 6;
	}

	mesh->indices.resize(indexData.size());
	memcpy(mesh->indices.data(), indexData.data(), indexData.size() * sizeof(uint32_t));
	mesh->primitives.push_back({ 0, static_cast<uint32_t>(mesh->indices.size()), -1 });
	mesh->topology = StateTopology::List;

	return std::move(mesh);
}

}    // namespace OmegaEngine
}    // namespace OmegaEngine
