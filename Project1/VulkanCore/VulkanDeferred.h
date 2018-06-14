#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/vulkan_tools.h"
#include "ComponentManagers/LightComponentManager.h"
#include "VulkanCore/VkMemoryManager.h"

#include "glm.hpp"

class VkDescriptors;
class VulkanTexture;
class VulkanRenderPass;
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

	struct DeferredInfo
	{
		DeferredInfo() :
			cmdBuffer(VK_NULL_HANDLE)
		{}

		VulkanTexture *position;
		VulkanTexture *normal;
		VulkanTexture *albedo;
		VulkanTexture *bump;
		VulkanTexture *ao;
		VulkanTexture *metallic;
		VulkanTexture *roughness;
		VulkanTexture *depth;
		VulkanTexture *offscreen;		// used for the post-processing pipeline

		VulkanRenderPass *renderpass;
		VkCommandBuffer cmdBuffer;
		VkSemaphore semaphore;

		VulkanUtility::PipeLlineInfo pipelineInfo;
		VkDescriptors *descriptor;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	struct BufferInfo
	{
		VkMemoryManager::SegmentInfo vertexUbo;
		VkMemoryManager::SegmentInfo fragmentUbo;
		VkMemoryManager::SegmentInfo vertices;
		VkMemoryManager::SegmentInfo indices;
	};

	VulkanDeferred(VulkanEngine *engine, VkMemoryManager *memory);
	virtual ~VulkanDeferred();

	void Init();
	void Update(int acc_time) override;

	void PrepareDeferredImages();
	void PrepareDeferredFramebuffer();
	void PrepareDeferredDescriptorSet();
	void PrepareDeferredPipeline();
	void GenerateDeferredCmdBuffer(VkCommandBuffer cmdBuffer);
	void DrawDeferredScene();
	void CreateUBOBuffers();
	void PrepareFullscreenQuad();

	// helper function
	VkRenderPass GetRenderPass() const;
	VkImageView GetOffscreenImageView() const;
	VkSampler GetOffscreenSampler() const;

	friend class VulkanShadow;
	friend class VulkanEngine;
	friend class VulkanSkybox;

private:

	void Destroy() override;

	VulkanEngine * p_vkEngine;

	DeferredInfo m_deferredInfo;
	BufferInfo m_buffers;
};

