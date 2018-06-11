#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VkMemoryManager.h"
#include "VulkanCore/VkDescriptors.h"

class VulkanEngine;
class CameraSystem;
class VkMemoryManager;

class VulkanSkybox : public VulkanModule
{
public:

	struct SkyboxInfo
	{
		VkRenderPass renderpass;
		VulkanUtility::PipeLlineInfo pipeline;
		VkDescriptors descriptors;
		VkMemoryManager::SegmentInfo uboBuffer;
	};

	struct SkyboxUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
	};

	VulkanSkybox(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	~VulkanSkybox();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;
	void PrepareSkyboxDescriptorSets();
	void PrepareSkyboxPipeline();
	void GenerateSkyboxCmdBuffer(VkCommandBuffer cmdBuffer);

private:

	VulkanEngine * p_vkEngine;

	SkyboxInfo m_envCube;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;
};

