#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VkMemoryManager.h"
#include <vector>

class VulkanEngine;
class CameraSystem;
class VkMemoryManager;
class VkDescriptors;

class VulkanSkybox : public VulkanModule
{
public:

	struct SkyboxInfo
	{
		VulkanUtility::PipeLlineInfo pipeline;
		VkDescriptors *descriptors;
		VkMemoryManager::SegmentInfo uboBuffer;
	};

	struct SkyboxUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
	};

	VulkanSkybox(VulkanEngine *engine, VkMemoryManager *memory);
	virtual ~VulkanSkybox();

	void Init();
	void Update(int acc_time) override;
	
	void PrepareSkyboxDescriptorSets();
	void PrepareSkyboxPipeline();
	void GenerateSkyboxCmdBuffer(VkCommandBuffer cmdBuffer);

private:

	void Destroy() override;

	VulkanEngine * p_vkEngine;

	SkyboxInfo m_envCube;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;
};

