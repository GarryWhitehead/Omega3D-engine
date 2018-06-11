#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VulkanRenderPass.h"

class VulkanEngine;
class CameraSystem;

class VulkanPBR : public VulkanModule
{

public:

	const int LUT_DIM = 512;

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> Vertex::GetAttrBindingDescription();

		glm::vec2 uv;
	};

	VulkanPBR(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	virtual ~VulkanPBR();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;
	void PrepareLUTFramebuffer();
	void PrepareLUTPipeline();
	void GenerateLUTCmdBuffer();

	friend class VulkanDeferred;

private:

	VulkanEngine * p_vkEngine;

	VulkanTexture m_lutImage;
	VulkanRenderPass m_renderpass;
	VulkanUtility::PipeLlineInfo m_pipelineInfo;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_lutShader;

};

