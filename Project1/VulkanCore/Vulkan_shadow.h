#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VkMemoryManager.h"

#include <array>

class VulkanEngine;
class CameraSystem;
class VkMemoryManager;
class VkDescriptors;
class VulkanTexture;
class VulkanRenderPass;

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

	VulkanShadow(VulkanEngine* engine, VkMemoryManager *manager);
	virtual ~VulkanShadow();

	void Init();
	void Update(int acc_time) override;

	void PrepareShadowFrameBuffer();
	void PrepareShadowDescriptors();
	void PrepareShadowPipeline();
	void GenerateShadowCmdBuffer();

	friend class VulkanEngine;
	friend class VulkanTerrain;
	friend class VulkanModel;
	friend class VulkanDeferred;

private:

	void Destroy() override;

	struct OffscreenInfo
	{
		VulkanRenderPass *renderpass;
		VkSemaphore semaphore;

		VkDescriptors *descriptors;
		VulkanUtility::PipeLlineInfo pipelineInfo;

		VkMemoryManager::SegmentInfo uboBuffer;

		std::array<VkPipelineShaderStageCreateInfo, 3> shader;

	} m_shadowInfo;

	VulkanTexture *p_depthImage;
	VkCommandBuffer m_cmdBuffer;

	VulkanEngine *p_vkEngine;
};

