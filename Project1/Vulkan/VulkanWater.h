#pragma once
//
// 
#include <array>
#include "glm.hpp"
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/MemoryAllocator.h"


class VulkanEngine;
class vkFFT;
class CameraSystem;
class MemoryAllocator;
class VulkanTexture;
class VulkanRenderPass;
class VkDescriptors;

class VulkanWater : public VulkanModule
{

public:

	static const uint32_t PATCH_SIZE = 512;
	const float UV_SCALE = 1.0f;

	// tessellation shader factors
	const float TESSELLATION_DISP_FACTOR = 40.0f;	// not used!
	const float TESSELLATION_FACTOR = 0.7f;
	const float TESSELLATION_EDGE_SIZE = 20.0f;		// not used!

	// perlin noise constants
	const glm::vec4 PERLIN_GRADIENT = glm::vec4(1.4f, 1.6f, 2.2f, 0.0f);
	const glm::vec4 PERLIN_OCTAVE = glm::vec4(1.12f, 0.59f, 0.23f, 0.0f);
	const glm::vec4 PERLIN_AMPLITUDE = glm::vec4(35.0, 42.0, 57.0, 0.0f);
	const float PERLIN_SPEED = 0.1f;
	
	struct WaterInfo
	{
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VkDescriptors *descriptors;
		std::array<VkPipelineShaderStageCreateInfo, 4> shader;
		VkPipeline wirePipeline;			// seperate pipeline for the wireframe draw

		// buffers
		MemoryAllocator::SegmentInfo uboTese;
		MemoryAllocator::SegmentInfo uboFrag;
		MemoryAllocator::SegmentInfo vertices;
		MemoryAllocator::SegmentInfo indices;
		uint32_t indexCount;

		// textures
		VulkanTexture *noiseImage;
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

	VulkanWater(VulkanEngine *engine, MemoryAllocator *memory);
	virtual ~VulkanWater();
	
	void Init();
	void Update(int acc_time) override;

	// rendering functions
	void PrepareBuffers();
	void LoadWaterAssets();
	void PrepareDescriptorSets();
	void PreparePipeline();
	void GenerateWaterCmdBuffer(VkCommandBuffer cmdBuffer, bool drawShadow = false);

	// compute shader functions
	void SubmitWaterCompute();

	// mesh generation
	void GenerateMeshData();
	void GenerateH0Map();

private:

	void Destroy() override;

	VulkanEngine *p_vkEngine;
	vkFFT *p_FFT;
	
	WaterInfo m_waterInfo;
	WaterParams m_waterParams;

	// keep a local track of the elapsed time
	double app_time;
};

