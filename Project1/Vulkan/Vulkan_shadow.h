#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/MemoryAllocator.h"

#include <array>

class VulkanEngine;
class CameraSystem;
class MemoryAllocator;
class VkDescriptors;
class VulkanTexture;
class VulkanRenderPass;

class VulkanShadow : public VulkanModule
{

public:
	
	const uint32_t SHADOWMAP_SIZE = 2048;

	// projections values for offscreen buffer - from perspective of light
	const float zNear = 1.0f;
	const float zFar = 64.0f;

	// values used by vkCmdDepthBias to reduce mapping artefacts - dynamically set at draw time
	static constexpr float biasConstant = 1.25f;
	static constexpr float biasSlope = 1.75f;

	struct SsboBufferModel			// vertex shader
	{
		std::array<glm::mat4, 256> modelMatrix;
	};

	struct SsboBufferLight			// geometry shader
	{
		std::array<glm::mat4, 256> mvp;
	};

	struct PushConstant
	{
		uint32_t useModelIndex;
		uint32_t modelIndex;
	};


	VulkanShadow(VulkanEngine* engine, MemoryAllocator *manager);
	virtual ~VulkanShadow();

	void Init();
	void Update(int acc_time) override;

	void PrepareShadowFrameBuffer();
	void PrepareShadowDescriptors();
	void PrepareShadowPipeline();
	void GenerateShadowCmdBuffer(VkCommandBuffer cmdBuffer);

	// helper functions
	VkPipeline& GetPipeline() { return m_shadowInfo.pipelineInfo.pipeline; }
	VkPipelineLayout& GetPipelineLayout() { return m_shadowInfo.pipelineInfo.layout; }
	VkDescriptorSet& GetDescriptorSet();
	VkSampler& GetDepthSampler();
	VkImageView& GetDepthImageView();

	friend class VulkanEngine;

private:

	void Destroy() override;

	struct OffscreenInfo
	{
		VulkanRenderPass *renderpass;
		VkDescriptors *descriptors;
		VulkanUtility::PipeLlineInfo pipelineInfo;
		std::array<VkPipelineShaderStageCreateInfo, 3> shader;
		
	} m_shadowInfo;

	struct SsboBuffers
	{
		MemoryAllocator::SegmentInfo light;
		MemoryAllocator::SegmentInfo model;
	} ssboBuffer;

	VulkanTexture *p_depthImage;

	VulkanEngine *p_vkEngine;
};

