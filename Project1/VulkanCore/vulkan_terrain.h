#pragma once
#include "VulkanCore/VulkanModule.h"
#include <string>
#include <vector>
#include "glm.hpp"

class VulkanEngine;
class VulkanShadow;
class CameraSystem;

class VulkanTerrain : public VulkanModule
{
public:

	// tesselation patch values
	const uint32_t PATCH_SIZE = 64;		// 64 X 64 triangles
	const float UV_SCALE = 1.0f;
	const float WX = 2.0f;
	const float WY = 2.0f;

	// tessellation shader factors
	const float TESSELLATION_DISP_FACTOR = 40.0f;
	const float TESSELLATION_FACTOR = 0.75f;
	const float TESSELLATION_EDGE_SIZE = 20.0f;

	struct TerrainUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::vec2 screenDim;
		float disFactor;
		float tessFactor;
		float tessEdgeSize;
	};

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

	struct ImageInfo
	{
		TextureInfo terrain;
		TextureInfo heightMap;
	};

	struct TerrainData
	{
		struct BufferInfo
		{
			BufferData index;
			BufferData vertex;
			BufferData ubo;
			uint32_t indexCount;
		} buffer;

		struct Data
		{
			std::vector<TerrainUbo> uboData;
		} data;

		VulkanUtility::DescriptorInfo descrInfo;
		VulkanUtility::PipeLlineInfo pipeline;
		
	};

	VulkanTerrain(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanTerrain();

	void Init();
	void Update(CameraSystem *camera);
	void Destroy() override;

	void LoadTerrainTextures();
	void PrepareTerrainDescriptorSets();
	void PreparePipeline();
	void GenerateTerrainCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline = VK_NULL_HANDLE);
	void PrepareTerrainData();
	float GetHeightmapPixel(uint32_t x, uint32_t y);
	void PrepareUBOBuffer();

	friend class VulkanEngine;

protected:

	struct HeightmapInfo
	{
		uint16_t *data;
		uint32_t imageDim;
	} m_heightmapInfo;

	TerrainData m_terrainInfo;

	ImageInfo m_images;
	std::array<VkPipelineShaderStageCreateInfo, 4> m_shader;

	VulkanEngine *p_vkEngine;
};

