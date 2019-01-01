#pragma once
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/TerrainUtility.h"
#include "VulkanCore/MemoryAllocator.h"
#include <vector>
#include <string>
#include <vector>
#include "glm.hpp"

class VulkanEngine;
class VulkanShadow;
class CameraSystem;
class MemoryAllocator;
class VkDescriptors;
class VulkanTexture;
class VulkanRenderPass;

class VulkanTerrain : public VulkanModule
{
public:

	// tesselation patch values
	const uint32_t PATCH_SIZE = 128;		// 64 X 64 triangles
	const float UV_SCALE = 1.0f;

	struct ImageInfo
	{
		VulkanTexture *terrain;
		VulkanTexture *heightMap;
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
			MemoryAllocator::SegmentInfo index;
			MemoryAllocator::SegmentInfo vertex;
			MemoryAllocator::SegmentInfo ubo;
			uint32_t indexCount;
		} buffer;

		struct Data
		{
			std::vector<TerrainUbo> uboData;
		} data;

		VkDescriptors *descrInfo;
		VulkanUtility::PipeLlineInfo pipeline;
		VkPipeline wirePipeline;
		
	};

	VulkanTerrain(VulkanEngine *engine, MemoryAllocator *memory);
	virtual ~VulkanTerrain();

	void Init();
	void Update(int acc_time) override;

	void LoadTerrainTextures();
	void PrepareTerrainDescriptorSets();
	void PreparePipeline();
	void GenerateTerrainCmdBuffer(VkCommandBuffer cmdBuffer, bool drawShadow = false);
	void PrepareTerrainData();
	float GetHeightmapPixel(uint32_t x, uint32_t y);

	friend class VulkanEngine;

protected:

	void Destroy() override;

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

