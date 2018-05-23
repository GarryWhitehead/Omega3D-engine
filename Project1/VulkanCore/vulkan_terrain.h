#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/TerrainUtility.h"
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
	const uint32_t PATCH_SIZE = 128;		// 64 X 64 triangles
	const float UV_SCALE = 1.0f;

	// tessellation shader factors
	const float TESSELLATION_DISP_FACTOR = 75.0f;
	const float TESSELLATION_FACTOR = 0.75f;
	const float TESSELLATION_EDGE_SIZE = 40.0f;

	struct ImageInfo
	{
		VulkanTexture terrain;
		VulkanTexture heightMap;
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
			std::vector<TerrainUtility::TerrainUbo> uboData;
		} data;

		VulkanUtility::DescriptorInfo descrInfo;
		VulkanUtility::PipeLlineInfo pipeline;
		
	};

	VulkanTerrain(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanTerrain();

	void Init();
	void Update(int acc_time) override;
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

