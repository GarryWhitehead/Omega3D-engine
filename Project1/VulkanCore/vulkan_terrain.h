#pragma once
#include "VulkanCore/vulkan_utility.h"
#include <string>
#include <vector>
#include "glm.hpp"

class VulkanScene;
class CameraSystem;

class VulkanTerrain : public VulkanUtility
{
public:

	// tesselation patch values
	const uint32_t PATCH_SIZE = 64;		// 64 X 64 triangles
	const uint32_t UV_SCALE = 1.0f;
	const float WX = 2.0f;
	const float WY = 2.0f;

	// tessellation shader factors
	const float TESSELLATION_DISP_FACTOR = 32.0f;
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

	struct SkyboxUbo
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
	};

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 3> Vertex::GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	struct ImageInfo
	{
		TextureInfo terrain;
		TextureInfo heightMap;
		TextureInfo skybox;
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

		DescriptorInfo descrInfo;
		PipeLlineInfo pipeline;
	};

	struct SkyboxData	
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
			std::vector<SkyboxUbo> uboData;
		} data;

		DescriptorInfo descrInfo;
		PipeLlineInfo pipeline;
	};

	VulkanTerrain();
	VulkanTerrain(VulkanScene *vulkanScene);
	~VulkanTerrain();

	void Init();
	void Update(CameraSystem *camera);
	void LoadTerrainTextures();
	void PrepareTerrainDescriptorSets();
	void PrepareSkyboxDescriptorSets();
	void PreparePipeline();
	void GenerateCmdBuffers();
	void PrepareTerrainData();
	float GetHeightmapPixel(uint32_t x, uint32_t y);
	void PrepareUBOBuffer();

	friend class VulkanScene;

protected:

	struct HeightmapInfo
	{
		uint16_t *data;
		uint32_t imageDim;
	} m_heightmapInfo;

	TerrainData m_terrainInfo;
	SkyboxData m_skyboxInfo;

	ImageInfo m_images;
	std::vector<VkCommandBuffer> m_cmdBuffer;
	std::array<VkPipelineShaderStageCreateInfo, 4> m_shader;

	VulkanScene *p_vulkanScene;
};

