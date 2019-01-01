#pragma once
//
// 
// 

#include "VulkanCore/Vulkan_tools.h"
#include "VulkanCore/MemoryAllocator.h"

class VulkanEngine;
class VulkanTexture;
class VkDescriptors;

class vkFFT 
{

public:

	const float PI = 3.1415926535897932384626433832795f;
	const float PI_2 = 6.28318530718f;
	const float GRAVITY = 9.81f;

	static const int32_t N = 256;
	static const uint32_t OFFSET_DX = N * N;
	static const uint32_t OFFSET_DY = N * N * 2;
	static const uint32_t OFFSET_DZ = 0;

	struct FFTstageInfo
	{
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VkDescriptors *descriptors;
		VkPipelineShaderStageCreateInfo shader;
		MemoryAllocator::SegmentInfo uboBuffer;
	};

	struct ButterFlyPushData
	{
		float N;
		float log2N;
	};

	struct SpecUboBuffer
	{
		float time;
		float L;
		float N;
		float offset_dx;
		float offset_dy;
		float offset_dz;
	};

	struct FFTPushData
	{
		float N;
		float offset;			
		float pingpong;
		float direction;
		float stage;
	};

	struct DisplacementUbo
	{
		float N;
		float offset_dx;
		float offset_dy;
		float offset_dz;
		float choppyFactor;
		float _pad0;
	};

	struct NormalUbo
	{
		float choppiness;
		float gridLength;
	};

	vkFFT(VulkanEngine *engine, MemoryAllocator *memory);
	~vkFFT();

	void Update(float acc_time, uint32_t patchLength);
	void Init(uint32_t patchLength, float windSpeed, float amplitude, glm::vec2 windDir);
	void GenerateSpectrumCmdBuffer(VkCommandBuffer cmdBuffer);
	void GenerateButterflyCmdBuffer();
	void GenerateFFTCmdBuffer(VkCommandBuffer cmdBuffer);
	void GeneratDisplacementCmdBuffer(VkCommandBuffer cmdBuffer);
	void GenerateNormalCmdBuffer(VkCommandBuffer cmdBuffer);
	void PrepareSpecDescriptorSets();
	void PrepareButterflyDescriptorSets();
	void PrepareFFTDescriptorSets();
	void PrepareDisplacementDescriptorSets();
	void PrepareNormalDescriptorSets();
	void PreparePipelines();
	void CreateBuffers();
	void SubmitFFTCompute();

	// h0(k) generation functions
	float GeneratePhillips(glm::vec2 k, float windSpeed, float amplitude, glm::vec2 windDir);
	void GenerateHeightMap(uint32_t patchLength, float windSpeed, float amplitude, glm::vec2 windDir);
	void GenerateBitReversed();

	friend class VulkanWater;

private:

	void Destroy();			// private to ensure the class isn't destroyed twice

	VulkanEngine *p_vkEngine;
	MemoryAllocator *p_vkMemory;

	// buffers for h0(k) and h0(-k)
	MemoryAllocator::SegmentInfo h0k_map;
	MemoryAllocator::SegmentInfo h0minusk_map;
	MemoryAllocator::SegmentInfo omega_map;
	MemoryAllocator::SegmentInfo bitrev_map;

	// ssbo buffers for d(x, y, z)
	MemoryAllocator::SegmentInfo ssbo_dxyz;

	// pingpong buffers
	MemoryAllocator::SegmentInfo ssbo_pingpong;

	// ubo buffers
	MemoryAllocator::SegmentInfo ubo_spectrum;
	MemoryAllocator::SegmentInfo ubo_displacement;
	MemoryAllocator::SegmentInfo ubo_normal;

	// pipeline data
	FFTstageInfo m_fftSpectrum;
	FFTstageInfo m_fftButterfly;
	FFTstageInfo m_fft;
	FFTstageInfo m_fftDisplacement;
	FFTstageInfo m_fftNormal;

	// texture data
	struct ImageInfo
	{
		VulkanTexture *butterfly;
		VulkanTexture *displacement;
		VulkanTexture *normal;
	} m_images;

	// command buffer and fences
	VkCommandBuffer m_computeCmdBuffer;
	VkFence m_computeFence;

	uint8_t m_pingpongID;
};

