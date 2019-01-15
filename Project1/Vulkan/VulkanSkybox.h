#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/MemoryAllocator.h"
#include <vector>

class VulkanEngine;
class CameraSystem;
class MemoryAllocator;
class VkDescriptors;

class VulkanSkybox : public VulkanModule
{
public:

	struct SkyboxInfo
	{
		VulkanUtility::PipeLlineInfo pipeline;
		VkDescriptors *descriptors;
		MemoryAllocator::MemorySegment uboBuffer;
	};

	struct SkyboxUbo
	{
		glm::mat4 projection;
		glm::mat4 modelMatrix;
	};

	VulkanSkybox(VulkanEngine *engine, MemoryAllocator *memory);
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

