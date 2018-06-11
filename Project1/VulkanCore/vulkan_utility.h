#pragma once
#include <vector>
#include <array>
#include "VulkanCore/vulkan_tools.h"
#include "gli.hpp"

class VulkanEngine;

class VulkanUtility
{
public:

	static const bool VK_SINGLE_USE = true;
	static const bool VK_MULTI_USE = false;
	static const bool VK_PRIMARY = true;
	static const bool VK_SECONDARY = false;

	struct PipeLlineInfo
	{
		VkPipelineLayout layout;
		VkPipeline pipeline;
		VkPipelineCache cache;
	};

	struct ViewPortInfo
	{
		VkViewport viewPort;
		VkRect2D scissor;
	};

	VulkanUtility(VulkanEngine *engine);
	~VulkanUtility();

	void InitVulkanUtility(VulkanEngine *engine);

	// descriptor prep utilities
	uint32_t FindMemoryType(const uint32_t type, const VkMemoryPropertyFlags flags);
	void CopyBuffer(const VkBuffer dest, const VkBuffer src, const VkDeviceSize size, const VkCommandPool cmdPool, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	// command buffer utilities
	VkCommandPool InitCommandPool(const uint32_t index);
	VkCommandBuffer CreateCmdBuffer(bool primary, bool singleUse, const VkFramebuffer frameBuffer, const VkRenderPass renderPass, const VkCommandPool cmdPool);
	void SubmitCmdBufferToQueue(const VkCommandBuffer cmdBuffer, const VkQueue queue, const VkCommandPool cmdPool);
	bool CheckForCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer);

	// pipeline utilites
	VkPipelineViewportStateCreateInfo InitViewPortCreateInfo(VkViewport& viewPort, VkRect2D& scissor, const uint32_t width, const uint32_t height);
	VkViewport InitViewPort(const uint32_t width, const uint32_t height, const float minDepth, const float maxDepth);
	VkRect2D InitScissor(const uint32_t width, const uint32_t height, const uint32_t x, const uint32_t y);
	VkPipelineRasterizationStateCreateInfo InitRasterzationState(const VkPolygonMode polyMode, const VkCullModeFlagBits cullMode, const VkFrontFace frontFace);
	VkPipelineMultisampleStateCreateInfo InitMultisampleState(const VkSampleCountFlagBits flag);

	// shader utilites
	VkPipelineShaderStageCreateInfo InitShaders(std::string shaderFile, const VkShaderStageFlagBits stage);
	void LoadFile(std::string filename, std::vector<char>& data);
	VkShaderModule CreateShaderModule(std::vector<char>& shader);
	VkPipelineShaderStageCreateInfo CreateShader(const VkShaderModule shader, const VkShaderStageFlagBits stage);

	// render
	uint32_t InitRenderFrame();
	void SubmitFrame(const uint32_t imageIndex);

	// image
	void ImageTransition(const VkCommandBuffer cmdBuff, const VkImage image, const VkFormat format, const VkImageLayout old_layout, const VkImageLayout new_layout, const VkCommandPool cmdPool, uint32_t mipLevels = 1, uint32_t layers = 1);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& requiredFormats, const VkImageTiling tiling, const VkFormatFeatureFlags features);

protected:

	VulkanEngine *p_vkEngine;
};



