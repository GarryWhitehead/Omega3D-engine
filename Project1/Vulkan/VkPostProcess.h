#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/VkMemoryManager.h"
#include "glm.hpp"
#include <array>

class VulkanEngine;
class VkDescriptors;
class VulkanTexture;
class VulkanRenderPass;

class VkPostProcess : public VulkanModule
{

public:

	const float BLUR_SCALE = 1.0f;
	const float BLUR_STRENGTH = 1.0f;
	const uint32_t BLUR_FB_DIM = 256;
	const uint32_t BLUR_MIP_LEVELS = 4;

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
		float exposure;			// tone mapping variables
		float gamma;
		float _pad0;
		uint32_t enableFog;
	};

	struct PostProcessVS		// the same vert ubo buffer is used for each pass
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
		glm::mat4 camModelView;
	};

	struct BlurUbo
	{
		float blurStrength;
		float blurScale;
	};

	struct PostProcessInfo
	{
		VkDescriptors *descriptors;
		VulkanRenderPass *renderpass;
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VkMemoryManager::SegmentInfo uboBuffer;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	struct BlurInfo
	{
		VkDescriptors *descriptors;
		VulkanRenderPass *renderpassHoriz;
		VulkanRenderPass *renderpassVert;
		VulkanUtility::PipeLlineInfo pipelineInfo;
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
		VkMemoryManager::SegmentInfo uboBuffer;
	};
	
	struct DebugInfo
	{
		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
		VulkanUtility::PipeLlineInfo pipelineInfo;
		VkDescriptors *descriptors;
	};

	VkPostProcess(VulkanEngine *engine, VkMemoryManager *memory);
	virtual ~VkPostProcess();

	void Init() override;
	void Update(int acc_time) override;
	void Destroy() override;
	
	void PrepareBuffers();
	void PrepareFrameBuffers();
	void PreparePipelines();
	void PrepareColourPassDescriptors();
	void PrepareBlurPassDescriptors();
	void PrepareFinalDescriptors();
	void PrepareDebugDescriptors();
	void GenerateColPassCmdBuffer(VkCommandBuffer cmdBuffer);
	void GenerateBlurPassCmdBuffer(VkCommandBuffer& cmdBuffer, VkRenderPass renderpass, VkFramebuffer framebuffer, uint32_t width, uint32_t height, uint32_t direction);
	void GenerateFinalCmdBuffer(VkCommandBuffer cmdBuffer);
	void GenerateDebugCmdBuffer(VkCommandBuffer cmdBuffer);
	void DrawBloom();
	void PrepareFullscreenQuad();
	void Submit(VkSemaphore *last_semaphore);

	// helper functions
	VkSemaphore& GetOffscreenSemaphore() { return offscreen_semaphore; }

private:

	VulkanEngine *p_vkEngine;

	// fog data
	PostProcessInfo m_ppInfo;
	PostProcessInfo m_finalInfo;
	BlurInfo m_bloomInfo;
	DebugInfo m_debugInfo;
		 
	// buffers for vertices, etc.
	VkMemoryManager::SegmentInfo m_vertices;
	VkMemoryManager::SegmentInfo m_indices;
	VkMemoryManager::SegmentInfo m_vertBuffer;		// vertex ubo buffer 

	VkCommandBuffer offscreen_cmdBuffer;
	VkSemaphore offscreen_semaphore;

	// texture samplers for bloom/normal scene
	struct Images
	{
		VulkanTexture *normalCol;
		VulkanTexture *brightCol;
		VulkanTexture *bloom;
		VulkanTexture *depth;
	} m_images;
};

