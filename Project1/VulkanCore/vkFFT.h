#pragma once
//
// This work is based upon the NVIDIA SDK Dx11 code which can be found at:
// 

#include "VulkanCore/Vulkan_Utility.h"
#include "VulkanCore/VulkanBuffer.h"

class VulkanEngine;

class vkFFT 
{

public:

	const float PI_2 = 6.28318530718f;

	struct UboData
	{
		float i_stride;
		float o_stride;
		float p_stride;
		float phaseBase;		// for twiddle calculations
	};

	struct UboLayout
	{
		UboData data[6];
	};
	
	
	vkFFT(VulkanEngine *engine);
	~vkFFT();

	// using complex-complex radix-8 FFT for now TODO: convert to C2R
	void GenerateFFTCmdBuffer(VkBuffer &srcBuffer);
	void PrepareFFTDescriptorSets(VulkanBuffer &srcBuffer);
	void PrepareFFTPipeline();
	void CreateBuffers();
	void SubmitFFTCompute();

	friend class VulkanWater;

private:

	VulkanEngine *p_vkEngine;

	// pipeline data
	VulkanUtility::PipeLlineInfo pipelineInfo;
	VulkanUtility::DescriptorInfo descriptors;
	VkPipelineShaderStageCreateInfo shader;
	VkCommandBuffer m_cmdBuffer;
	VkFence m_fence;

	// buffers for compute shader
	VulkanBuffer m_uboBuffer;
	VulkanBuffer m_ssboDstBuffer;	// d(z)	
};

