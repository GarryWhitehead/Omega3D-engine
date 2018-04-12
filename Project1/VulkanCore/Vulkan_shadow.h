#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanDeferred.h"
#include <array>

class VulkanEngine;
class CameraSystem;

class VulkanShadow : public VulkanModule
{

public:
	
	const uint32_t SHADOWMAP_SIZE = 2048;

	// projections values for offscreen buffer - from perspective of light
	const float zNear = 1.0f;
	const float zFar = 96.0f;

	// values used by vkCmdDepthBias to reduce mapping artefacts - dynamically set at draw time
	float biasConstant = 1.25f;
	float biasSlope = 1.75f;

	struct ShadowUbo
	{
		glm::mat4 mvp[LIGHT_COUNT];
		glm::vec4 lightOffsets[LIGHT_COUNT];
	};

	VulkanShadow(VulkanEngine* engine, VulkanUtility *utility);
	~VulkanShadow();

	void Init();
	void Update(CameraSystem *camera);
	void Destroy() override;

	void PrepareShadowPass();
	void PrepareShadowDescriptors();
	void PrepareShadowRenderpass();
	void PrepareShadowPipeline();
	void GenerateShadowCmdBuffer(VkCommandBuffer cmdBuffer);
	void PrepareUBOBuffer();

	friend class VulkanEngine;
	friend class VulkanTerrain;
	friend class VulkanModel;
	friend class VulkanDeferred;

private:

	struct OffscreenInfo
	{
		VkFramebuffer frameBuffer;
		VkRenderPass renderpass;
		VkSemaphore semaphore;

		VulkanUtility::DescriptorInfo descriptors;
		VulkanUtility::PipeLlineInfo pipelineInfo;

		BufferData uboBuffer;
		ShadowUbo uboData;

		std::array<VkPipelineShaderStageCreateInfo, 3> shader;

	} m_shadowInfo;

	TextureInfo m_depthImage;

	VulkanEngine *p_vkEngine;
};

