#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/TerrainUtility.h"
#include "VulkanCore/VkMemoryManager.h"
#include "VulkanCore/VkDescriptors.h"
#include <string>
#include <vector>
#include "glm.hpp"

class VulkanEngine;
class VulkanShadow;
class CameraSystem;
class VkMemoryManager;

class VulkanTerrain : public VulkanModule
{
public:

	// tesselation patch values
	const uint32_t PATCH_SIZE = 128;		// 64 X 64 triangles
	const float UV_SCALE = 1.0f;

	struct ImageInfo
	{
		VulkanTexture terrain;
		VulkanTexture heightMap;
	};

	struct TerrainUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::vec2 screenDim;
		float disFactor;
		float tessFactor;
		float tessEdgeSize;
		float _pad0;
	};

	struct TerrainData
	{
		struct BufferInfo
		{
			VkMemoryManager::SegmentInfo index;
			VkMemoryManager::SegmentInfo vertex;
			VkMemoryManager::SegmentInfo ubo;
			uint32_t indexCount;
		} buffer;

		struct Data
		{
			std::vector<TerrainUbo> uboData;
		} data;

		VkDescriptors descrInfo;
		VulkanUtility::PipeLlineInfo pipeline;
		VkPipeline wirePipeline;
		
	};

	VulkanTerrain(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	virtual ~VulkanTerrain();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;

	void LoadTerrainTextures();
	void PrepareTerrainDescriptorSets();
	void PreparePipeline();
	void GenerateTerrainCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline = VK_NULL_HANDLE);
	void PrepareTerrainData();
	float GetHeightmapPixel(uint32_t x, uint32_t y);

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

