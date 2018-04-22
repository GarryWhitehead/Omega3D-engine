#pragma once
#include "VulkanCore/VulkanModule.h"
#include "glm.hpp"

class VulkanEngine;
class CameraSystem;

const int LIGHT_COUNT = 3;

class VulkanDeferred : public VulkanModule
{

public:

	static const int DEFERRED_SIZE = 2048;
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

	// TODO: make a light component and store all this information in the manager
	struct LightInfo
	{
		LightInfo() {}
		LightInfo(glm::vec4 p, glm::vec4 dir, glm::vec4 col) :
			pos(p),
			target(dir),
			colour(col)
		{}

		glm::vec4 pos;
		glm::vec4 target;
		glm::vec4 colour;
		glm::mat4 viewMatrix;
	};

	struct FragmentUBOLayout
	{
		glm::vec4 viewPos;
		LightInfo lights[LIGHT_COUNT];
		glm::vec4 cameraPos;
	};

	struct DeferredBufferInfo
	{
		TextureInfo imageInfo;
	};

	struct DeferredInfo
	{
		DeferredBufferInfo position;
		DeferredBufferInfo normal;
		DeferredBufferInfo albedo;
		DeferredBufferInfo ao;
		DeferredBufferInfo metallic;
		DeferredBufferInfo roughness;
		DeferredBufferInfo depth;

		VkFramebuffer frameBuffer;
		VkRenderPass renderPass;
	
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VulkanUtility::DescriptorInfo descriptor;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	struct ForwardInfo
	{
		TextureInfo offscreenImage;
		TextureInfo imageInfo;
		VkFramebuffer framebuffer;
		VkRenderPass renderPass;
	};

	struct BufferInfo
	{
		BufferData vertexUbo;
		BufferData fragmentUbo;
		BufferData vertices;
		BufferData indices;
	};

	VulkanDeferred(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanDeferred();

	void Init();
	void Update(CameraSystem *camera);
	void Destroy() override;
	void CreateRenderpassAttachmentInfo(VkImageLayout finalLayout, VkFormat format, const uint32_t attachCount, VkAttachmentDescription *attachDescr, VkAttachmentReference *attachRef);
	void CreateDeferredImage(const VkFormat format, VkImageUsageFlagBits usageFlags, TextureInfo& imageInfo);
	void PrepareDeferredFramebuffer();
	void PrepareDeferredRenderpass();
	void PrepareDeferredDescriptorSet();
	void PrepareDeferredPipeline();
	void GenerateDeferredCmdBuffer(VkCommandBuffer cmdBuffer);
	void GenerateFullscreenCmdBuffers();
	void CreateUBOBuffers();
	void PrepareFullscreenQuad();
	void PreapareLightData();

	// forward renderpass fucntions
	void PrepareForwardFramebuffer();
	void GenerateForwardCmdBuffer(VkCommandBuffer cmdBuffer);

	// helper function
	VkRenderPass GetRenderPass() const { return m_deferredInfo.renderPass; }

	friend class VulkanShadow;
	friend class VulkanEngine;
	friend class VulkanSkybox;

private:

	VulkanEngine * p_vkEngine;

	DeferredInfo m_deferredInfo;
	ForwardInfo m_forwardInfo;

	BufferInfo m_buffers;
	FragmentUBOLayout m_fragBuffer;
	VertexUBOLayout m_vertBuffer;

	std::vector<VkCommandBuffer> m_cmdBuffers;
};

