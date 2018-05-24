#pragma once
#include "VulkanCore/VulkanModule.h"
#include "ComponentManagers/LightComponentManager.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/VkMemoryManager.h"
#include "glm.hpp"

class VulkanEngine;
class CameraSystem;
class VkMemoryManager;

class VulkanDeferred : public VulkanModule
{

public:

	static const VkSampleCountFlagBits SAMPLE_COUNT = VK_SAMPLE_COUNT_4_BIT;

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 4> Vertex::GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 colour;
	};

	struct VertexUBOLayout
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
	};

	struct FragLightInfo
	{
		glm::vec4 pos;
		glm::vec4 direction;
		glm::vec4 colour;
	};

	struct FragmentUBOLayout
	{
		glm::vec4 cameraPos;
		FragLightInfo light[LightComponentManager::MAX_LIGHT_COUNT];
		uint32_t activeLightCount;
		
	};

	struct DeferredBufferInfo
	{
		VulkanTexture imageInfo;
	};

	struct DeferredInfo
	{
		DeferredBufferInfo position;
		DeferredBufferInfo normal;
		DeferredBufferInfo albedo;
		DeferredBufferInfo bump;
		DeferredBufferInfo ao;
		DeferredBufferInfo metallic;
		DeferredBufferInfo roughness;
		DeferredBufferInfo depth;

		std::vector<VkFramebuffer> frameBuffers;
		VkRenderPass renderPass;
	
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VulkanUtility::DescriptorInfo descriptor;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	struct BufferInfo
	{
		VkMemoryManager::SegmentInfo vertexUbo;
		VkMemoryManager::SegmentInfo fragmentUbo;
		VkMemoryManager::SegmentInfo vertices;
		VkMemoryManager::SegmentInfo indices;
	};

	VulkanDeferred(VulkanEngine *engine, VulkanUtility *utility, VkMemoryManager *memory);
	~VulkanDeferred();

	void Init();
	void Update(int acc_time) override;
	void Destroy() override;

	void CreateRenderpassAttachmentInfo(VkImageLayout finalLayout, VkFormat format, const uint32_t attachCount, VkAttachmentDescription *attachDescr);
	void PrepareDeferredFramebuffer();
	void PrepareDeferredRenderpass();
	void PrepareDeferredDescriptorSet();
	void PrepareDeferredPipeline();
	void GenerateDeferredCmdBuffer(VkCommandBuffer cmdBuffer);;
	void CreateUBOBuffers();
	void PrepareFullscreenQuad();

	// helper function
	VkRenderPass GetRenderPass() const { return m_deferredInfo.renderPass; }

	friend class VulkanShadow;
	friend class VulkanEngine;
	friend class VulkanSkybox;

private:

	VulkanEngine * p_vkEngine;

	DeferredInfo m_deferredInfo;
	BufferInfo m_buffers;
};

