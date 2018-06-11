#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/vulkan_utility.h"
#include "VulkanCore/VkMemoryManager.h"
#include "VulkanCore/VulkanRenderpass.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VkDescriptors.h"
#include "glm.hpp"

class VulkanEngine;

class VkPostProcess : public VulkanModule
{

public:

	const float RAY_DIRECTION = 0.8f;
	const float SUN_DIRECTION = 0.5f;
	const float FOG_DENSITY = 0.05f;

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 2> Vertex::GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
	};

	struct FogUboFS
	{			
		float rayDir;
		float sunDir;
		float fogDensity;
		uint32_t enableFog;
	};

	struct FogUboVS
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 camModelView;
	};

	struct PostProcessInfo
	{
		VkDescriptors descriptors;
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VkMemoryManager::SegmentInfo uboBufferVS;
		VkMemoryManager::SegmentInfo uboBufferFS;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	VkPostProcess(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	virtual ~VkPostProcess();

	void Init() override;
	void Update(int acc_time) override;
	void Destroy() override;
	
	void PreparePipeline();
	void PrepareDescriptors();
	void GenerateCmdBuffer(VkCommandBuffer cmdBuffer);
	void PrepareFullscreenQuad();

private:

	VulkanEngine *p_vkEngine;

	// fog data
	PostProcessInfo m_fogInfo;

	// buffers for vertices, etc.
	VkMemoryManager::SegmentInfo m_vertices;
	VkMemoryManager::SegmentInfo m_indices;
};

