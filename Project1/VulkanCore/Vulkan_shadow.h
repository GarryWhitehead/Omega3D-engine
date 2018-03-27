#pragma once
#include "VulkanCore/VulkanModule.h"
#include <array>

class VulkanEngine;
class CameraSystem;

class VulkanShadow : public VulkanModule
{

public:
	
	const uint32_t SHADOWMAP_WIDTH = 4096;
	const uint32_t SHADOWMAP_HEIGHT = 4096;

	// projections values for offscreen buffer - from perspective of light
	const float zNear = 1.0f;
	const float zFar = 96.0f;

	// values used by vkCmdDepthBias to reduce mapping artefacts - dynamically set at draw time
	float biasConstant = 1.25f;
	float biasSlope = 1.75f;

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 1> Vertex::GetAttrBindingDescription();
		
		glm::vec3 pos;
		glm::vec2 uv;
	};

	struct OffscreenUbo
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
	};

	VulkanShadow();
	VulkanShadow(VulkanEngine* engine);
	~VulkanShadow();

	void Init();
	void Update(CameraSystem *camera);
	void Destroy() override;

	void PrepareDepthBuffer();
	void PrepareOffscreenDescriptors();
	void PrepareOffscreenRenderpass();
	void PrepareOffscreenPipeline();
	void GenerateOffscreenCmdBuffer();
	void PrepareUBOBuffer();

	friend class VulkanEngine;
	friend class VulkanTerrain;
	friend class VulkanModel;

private:

	struct OffscreenInfo
	{
		VkFramebuffer frameBuffer;
		VkCommandBuffer cmdBuffer;
		VkRenderPass renderpass;
		VkSemaphore semaphore;

		VulkanUtility::DescriptorInfo descriptors;
		VulkanUtility::PipeLlineInfo pipelineInfo;

		BufferData uboBuffer;
		OffscreenUbo uboData;

		std::array<VkPipelineShaderStageCreateInfo, 2> shader;

	} m_offscreenInfo;

	TextureInfo m_depthImage;

	VulkanEngine *p_vkEngine;
};

