#pragma once
#include "VulkanCore/VulkanModule.h"

class VulkanEngine;
class CameraSystem;

class VulkanSkybox : public VulkanModule
{
public:

	struct SkyboxInfo
	{
		VkRenderPass renderpass;
		VulkanUtility::PipeLlineInfo pipeline;
		VulkanUtility::DescriptorInfo descriptors;
		BufferData uboBuffer;
	};

	struct SkyboxUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
	};

	VulkanSkybox(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanSkybox();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;
	void PrepareSkyboxDescriptorSets();
	void PrepareSkyboxPipeline();
	void GenerateSkyboxCmdBuffer(VkCommandBuffer cmdBuffer);
	void PrepareUboBuffer();

private:

	VulkanEngine * p_vkEngine;

	SkyboxInfo m_envCube;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;
};

