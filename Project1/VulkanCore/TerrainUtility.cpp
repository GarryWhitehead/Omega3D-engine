#include "TerrainUtility.h"



TerrainUtility::TerrainUtility()
{
}


TerrainUtility::~TerrainUtility()
{
}

void TerrainUtility::GenerateVertices(uint32_t patchSize, float uvFactor, std::vector<Vertex>& vertices)
{
	const float widthX = 2.0f;
	const float widthY = 2.0f;

	const uint32_t vertCount = patchSize * patchSize;
	vertices.resize(vertCount);

	for (uint32_t x = 0; x < patchSize; ++x) {

		for (uint32_t y = 0; y < patchSize; ++y) {

			uint32_t index = x + y * patchSize;
			vertices[index].pos.x = x * widthX + widthX / 2.0f - (float)patchSize * widthX / 2.0f;
			vertices[index].pos.z = y * widthY + widthY / 2.0f - (float)patchSize * widthY / 2.0f;
			vertices[index].pos.y = 0.0f;
			vertices[index].uv = glm::vec2(x / (float)patchSize, y / (float)patchSize) * uvFactor;
		}
	}
}

void TerrainUtility::GenerateIndices(uint32_t patchSize, std::vector<uint32_t>& indices)
{
	// generate the indices for the patch quads
	uint32_t width = patchSize - 1;
	uint32_t size = width * width * 4;

	indices.resize(size);

	for (uint32_t x = 0; x < width; ++x) {

		for (uint32_t y = 0; y < width; ++y) {

			uint32_t index = (x + y * width) * 4;
			indices[index] = (x + y * patchSize);				// top-left
			indices[index + 1] = indices[index] + patchSize;	// bottom-left
			indices[index + 2] = indices[index + 1] + 1;		//	bottom-right
			indices[index + 3] = indices[index] + 1;			// top-right
		}
	}
}

VkVertexInputBindingDescription TerrainUtility::Vertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(Vertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

// vertex attributes for main and background scene
std::array<VkVertexInputAttributeDescription, 6> TerrainUtility::Vertex::GetAttrBindingDescription()
{
	// Vertex layout 0: uv
	std::array<VkVertexInputAttributeDescription, 6> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(Vertex, pos);

	// Vertex layout 1: normal
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(Vertex, uv);

	// Vertex layout 2: colour
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(Vertex, normal);

	// Vertex layout 2: colour
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(Vertex, colour);

	// Vertex layout 5: bone weights
	attrDescr[4].binding = 0;
	attrDescr[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attrDescr[4].location = 4;
	attrDescr[4].offset = offsetof(Vertex, boneWeigthts);

	// Vertex layout 6: bone ids
	attrDescr[5].binding = 0;
	attrDescr[5].format = VK_FORMAT_R32G32B32_SINT;
	attrDescr[5].location = 5;
	attrDescr[5].offset = offsetof(Vertex, boneId);

	return attrDescr;
}