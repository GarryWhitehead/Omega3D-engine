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
	
	// ================================================================ constants ================================================================

	const float PI = 3.1415926535897932384626433832795f;

	const float GRAVITY = 981.0f;
	static const int32_t HEIGHTMAP_DIM = 512;
	static const int32_t DISPLACEMENT_MAP_SIZE = 512;
	const int32_t TOTAL_DISPLACEMENT_SIZE = DISPLACEMENT_MAP_SIZE * DISPLACEMENT_MAP_SIZE;
	static const uint32_t PATCH_SIZE = 256;
	const uint32_t BLOCK_SIZE = 16;
	const float UV_SCALE = 1.0f;

	// tessellation shader factors
	const float TESSELLATION_DISP_FACTOR = 40.0f;
	const float TESSELLATION_FACTOR = 0.75f;
	const float TESSELLATION_EDGE_SIZE = 20.0f;

	// perlin noise constants
	const glm::vec4 PERLIN_GRADIENT = glm::vec4(1.4f, 1.6f, 2.2f, 0.0f);
	const glm::vec4 PERLIN_OCTAVE = glm::vec4(1.12f, 0.59f, 0.23f, 0.0f);
	const glm::vec4 PERLIN_AMPLITUDE = glm::vec4(35.0, 42.0, 57.0, 0.0f);
	const float PERLIN_SPEED = 0.06f;

	// =========================================================== compute structs ==============================================================
	struct SpecComputeInfo
	{
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VulkanUtility::DescriptorInfo descriptors;
		VkPipelineShaderStageCreateInfo shader;
		VkCommandBuffer cmdBuffer;
		VkFence fence;
		
		struct BufferSSBO
		{
			VkMemoryManager::SegmentInfo ht;
			VkMemoryManager::SegmentInfo dx;
			VkMemoryManager::SegmentInfo dy;
		} ssboBuffer;

		VkMemoryManager::SegmentInfo uboBuffer;
	};

	struct DispComputeInfo
	{
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VulkanUtility::DescriptorInfo descriptors;
		VkPipelineShaderStageCreateInfo shader;
		VkCommandBuffer cmdBuffer;
		VkFence fence;

		VulkanTexture image;
		VkMemoryManager::SegmentInfo uboBuffer;
	};

	struct ComputeUbo
	{
		float time;
		uint32_t dismapDim;
		float _pad;
	};

	// =========================================================== offscreen definitions ===============================================================

	struct OffscreenInfo
	{
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VulkanUtility::DescriptorInfo descriptors;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
		VulkanTexture image;
		VulkanRenderPass renderpass;
		VkCommandBuffer cmdBuffer;
	};

	// ubo buffers
	struct DisplacementUbo
	{
		float choppiness;
		uint32_t mapSize;
		float _pad0;
		float _pad1;
	};

	struct NormalUbo
	{
		float choppiness;
		float gridLength;
		float _pad0;
		float _pad1;
	};

	// =================================================== main rendering variables ===================================================================
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
		glm::vec2 screenDim;
		float dispFactor;
		float tessFactor;
		float tessEdgeSize;
		float perlinMovement;
	};

	struct TerrainFragUbo
	{
		glm::vec4 perlinOctave;
		glm::vec4 perlinGradient;
		float perlinMovement;
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

	//  offscreen functions - generate normal map
	void PrepareOffscreenFrameBuffer();
	void PrepareOffscreenBuffers();
	void PrepareOffscreenDescriptorSets();
	void PrepareOffscreenPipeline();
	void GenerateOffscreenCmdBuffer(const VkCommandBuffer cmdBuffer);

	// rendering functions
	void PrepareBuffers();
	void LoadWaterAssets();
	void PrepareDescriptorSets();
	void PreparePipeline();
	void GenerateWaterCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline);

	// compute shader functions
	void PrepareComputeBuffers();
	void PrepareSpecComputeDescriptors();
	void PrepareDispComputeDescriptorSets();
	void PrepareSpecComputePipeLine();
	void PrepareDispComputePipeLine();
	void GenerateSpecComputeCmdBuffers();
	void GenerateDispComputeCmdBuffers();
	void SubmitWaterCompute();

	// heightmap generation functions
	float GeneratePhillips(glm::vec2 k);
	void GenerateHeightMap();

	// mesh generation
	void GenerateMeshData();


private:

	VulkanEngine *p_vkEngine;
	vkFFT *p_FFT;
	
	WaterInfo m_waterInfo;
	WaterParams m_waterParams;

	// compute shader setup variables
	SpecComputeInfo m_specComputeInfo;
	DispComputeInfo m_dispComputeInfo;

	// pipeline data for normal and displacement map updates
	OffscreenInfo m_normalInfo;
	OffscreenInfo m_displacementInfo;

	struct Buffers
	{
		// buffers for heightmaps and omega values
		VkMemoryManager::SegmentInfo heightMap;
		VkMemoryManager::SegmentInfo omegaMap;

		struct Offscreen
		{
			VkMemoryManager::SegmentInfo normUbo;
			VkMemoryManager::SegmentInfo dispUbo;
		} offscreen;

	} m_buffers;
};

