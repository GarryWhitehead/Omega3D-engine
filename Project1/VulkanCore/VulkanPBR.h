#pragma once
#include "VulkanCore/VulkanModule.h"

class VulkanEngine;

class VulkanPBR : public VulkanModule
{

public:

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> Vertex::GetAttrBindingDescription();

		glm::vec2 uv;
	};

	const int LUT_DIM = 512;

	VulkanPBR(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanPBR();

	void Init();
	void Destroy() override;
	void PrepareLUTImage();
	void PrepareLUTRenderpass();
	void PrepareLUTFramebuffer();
	void PrepareLUTPipeline();
	void GenerateLUTCmdBuffer();

	friend class VulkanDeferred;

private:

	VulkanEngine * p_vkEngine;

	TextureInfo lutImage;
	VkRenderPass m_lutRenderpass;
	VkFramebuffer m_lutFramebuffer;
	VkPipelineLayout m_lutLayout;
	VkPipeline m_lutPipeline;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_lutShader;

};

