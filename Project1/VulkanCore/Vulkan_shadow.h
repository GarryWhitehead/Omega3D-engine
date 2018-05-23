#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VulkanRenderpass.h"
#include "VulkanCore/VulkanBuffer.h"
#include <array>

class VulkanEngine;
class CameraSystem;

class VulkanShadow : public VulkanModule
{

public:
	
	const uint32_t SHADOWMAP_SIZE = 2048;
	static const uint32_t CSM_COUNT = 5;

	// projections values for offscreen buffer - from perspective of light
	const float zNear = 1.0f;
	const float zFar = 96.0f;

	// values used by vkCmdDepthBias to reduce mapping artefacts - dynamically set at draw time
	float biasConstant = 1.25f;
	float biasSlope = 1.75f;

	struct UboLayout
	{
		glm::mat4 mvp[256];
	};

	VulkanShadow(VulkanEngine* engine, VulkanUtility *utility);
	~VulkanShadow();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;

	void PrepareShadowFrameBuffer();
	void PrepareShadowDescriptors();
	void PrepareShadowPipeline();
	void GenerateShadowCmdBuffer(VkCommandBuffer cmdBuffer);
	void UpdateCSM();

	friend class VulkanEngine;
	friend class VulkanTerrain;
	friend class VulkanModel;
	friend class VulkanDeferred;

private:

	struct OffscreenInfo
	{
		VulkanRenderPass renderpass;
		VkSemaphore semaphore;

		VulkanUtility::DescriptorInfo descriptors;
		VulkanUtility::PipeLlineInfo pipelineInfo;

		VulkanBuffer uboBuffer;
		UboLayout uboData;

		std::array<VkPipelineShaderStageCreateInfo, 3> shader;

	} m_shadowInfo;

	VulkanTexture m_depthImage;
	std::vector<VkImageView> m_csmImageViews;
	std::vector<VkFramebuffer> m_csmFrameBuffers;

	VulkanEngine *p_vkEngine;
};

