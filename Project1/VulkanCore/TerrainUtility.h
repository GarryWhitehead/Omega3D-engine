#pragma once
#include "VulkanCore/VulkanModule.h"

// common functions used for all terrain generators that use tesselation shaders

class TerrainUtility
{

public:

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 6> Vertex::GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 colour;
		float boneWeigthts[4];
		uint32_t boneId[4];
	};

	TerrainUtility();
	~TerrainUtility();

	void GenerateVertices(uint32_t patchSize, float uvFactor, std::vector<Vertex>& vertices);
	void GenerateIndices(uint32_t patchSize, std::vector<uint32_t>& indices);

private:

};

