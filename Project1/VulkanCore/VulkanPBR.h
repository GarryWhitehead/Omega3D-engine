#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/VulkanTexture.h"

class VulkanEngine;
class CameraSystem;

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

	VulkanPBR(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	~VulkanPBR();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;
	void PrepareLUTRenderpass();
	void PrepareLUTFramebuffer();
	void PrepareLUTPipeline();
	void GenerateLUTCmdBuffer();

	friend class VulkanDeferred;

private:

	VulkanEngine * p_vkEngine;

	VulkanTexture lutImage;
	VkRenderPass m_lutRenderpass;
	VkFramebuffer m_lutFramebuffer;
	VkPipelineLayout m_lutLayout;
	VkPipeline m_lutPipeline;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_lutShader;

};

