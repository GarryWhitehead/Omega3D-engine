#pragma once
//
// 
#include <array>
#include "glm.hpp"
#include "VulkanCore/VulkanRenderPass.h"
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VkMemoryManager.h"

class VulkanEngine;
class vkFFT;
class CameraSystem;
class VkMemoryManager;

class VulkanWater : public VulkanModule
{

public:

	static const uint32_t PATCH_SIZE = 256;
	const float UV_SCALE = 1.0f;

	// tessellation shader factors
	const float TESSELLATION_DISP_FACTOR = 40.0f;
	const float TESSELLATION_FACTOR = 0.7f;
	const float TESSELLATION_EDGE_SIZE = 20.0f;

	// perlin noise constants
	const glm::vec4 PERLIN_GRADIENT = glm::vec4(1.4f, 1.6f, 2.2f, 0.0f);
	const glm::vec4 PERLIN_OCTAVE = glm::vec4(1.12f, 0.59f, 0.23f, 0.0f);
	const glm::vec4 PERLIN_AMPLITUDE = glm::vec4(35.0, 42.0, 57.0, 0.0f);
	const float PERLIN_SPEED = 0.06f;
	
	struct WaterInfo
	{
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VulkanUtility::DescriptorInfo descriptors;
		std::array<VkPipelineShaderStageCreateInfo, 4> shader;

		// buffers
		VkMemoryManager::SegmentInfo uboTese;
		VkMemoryManager::SegmentInfo uboFrag;
		VkMemoryManager::SegmentInfo vertices;
		VkMemoryManager::SegmentInfo indices;
		uint32_t indexCount;

		// textures
		VulkanTexture noiseImage;
	};

	struct TerrainTeseUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
		glm::mat4 viewMatrix;
		glm::vec4 cameraPos;
		glm::vec4 perlinOctave;
		glm::vec4 perlinAmplitude;
		glm::vec2 perlinMovement;
		glm::vec2 screenDim;
		float dispFactor;
		float tessFactor;
		float tessEdgeSize;
	};

	struct TerrainFragUbo
	{
		glm::vec4 perlinOctave;
		glm::vec4 perlinGradient;
		glm::vec2 perlinMovement;
		float _pad0;
		float texelSize2x;
	};

	struct WaterParams
	{
		float time;
		float amplitude;
		float windSpeed;
		float choppiness;
		glm::vec2 windDir;
		float patchLength;
	};

	VulkanWater(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	~VulkanWater();
	
	void Init();
	void Update(int acc_time) override;
	void Destroy() override;

	// rendering functions
	void PrepareBuffers();
	void LoadWaterAssets();
	void PrepareDescriptorSets();
	void PreparePipeline();
	void GenerateWaterCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline);

	// compute shader functions
	void SubmitWaterCompute();

	// mesh generation
	void GenerateMeshData();

private:

	VulkanEngine *p_vkEngine;
	vkFFT *p_FFT;
	
	WaterInfo m_waterInfo;
	WaterParams m_waterParams;
};

