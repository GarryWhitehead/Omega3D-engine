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
	};

	struct ViewPortInfo
	{
		VkViewport viewPort;
		VkRect2D scissor;
	};

	struct DescriptorInfo
	{
		VkDescriptorPool pool;
		VkDescriptorSetLayout layout;
		VkDescriptorSet set;
	};

	VulkanUtility(VulkanEngine *engine);
	~VulkanUtility();

	void InitVulkanUtility(VulkanEngine *engine);

	// descriptor prep utilities
	VkWriteDescriptorSet InitDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo *info);
	VkWriteDescriptorSet InitDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo *info);
	VkDescriptorImageInfo InitImageInfoDescriptor(VkImageLayout layout, VkImageView view, VkSampler sampler);
	VkDescriptorBufferInfo InitBufferInfoDescriptor(VkBuffer buffer, int offset, uint32_t range);
	VkDescriptorSetLayoutBinding InitLayoutBinding(int binding, VkDescriptorType type, VkShaderStageFlags flags);
	uint32_t FindMemoryType(uint32_t type, VkMemoryPropertyFlags flags);
	void CopyBuffer(VkBuffer dest, VkBuffer src, VkDeviceSize size, VkCommandPool cmdPool, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	// command buffer utilities
	std::vector<VkFramebuffer> InitFrameBuffers(uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView imageView);
	VkCommandPool InitCommandPool(uint32_t index);
	VkCommandBuffer CreateCmdBuffer(bool primary, bool singleUse, VkFramebuffer frameBuffer, VkRenderPass renderPass, VkCommandPool cmdPool);
	void SubmitCmdBufferToQueue(VkCommandBuffer cmdBuffer, VkQueue queue, VkCommandPool cmdPool);
	bool CheckForCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer);

	// pipeline utilites
	VkPipelineViewportStateCreateInfo InitViewPortCreateInfo(VkViewport& viewPort, VkRect2D& scissor, uint32_t width, uint32_t height);
	VkViewport InitViewPort(uint32_t width, uint32_t height, float minDepth, float maxDepth);
	VkRect2D InitScissor(uint32_t width, uint32_t height, uint32_t x, uint32_t y);
	VkPipelineRasterizationStateCreateInfo InitRasterzationState(VkPolygonMode polyMode, VkCullModeFlagBits cullMode, VkFrontFace frontFace);
	VkPipelineMultisampleStateCreateInfo InitMultisampleState(VkSampleCountFlagBits flag);

	// shader utilites
	VkPipelineShaderStageCreateInfo InitShaders(std::string shaderFile, VkShaderStageFlagBits stage);
	void LoadFile(std::string filename, std::vector<char>& data);
	VkShaderModule CreateShaderModule(std::vector<char>& shader);
	VkPipelineShaderStageCreateInfo CreateShader(VkShaderModule shader, VkShaderStageFlagBits stage);

	// render
	uint32_t InitRenderFrame();
	void SubmitFrame(uint32_t imageIndex);

	// image
	void ImageTransition(const VkCommandBuffer cmdBuff, const VkImage image, const VkFormat format, const VkImageLayout old_layout, const VkImageLayout new_layout, const VkCommandPool cmdPool, uint32_t mipLevels = 1, uint32_t layers = 1);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& requiredFormats, const VkImageTiling tiling, const VkFormatFeatureFlags features);

protected:

	VulkanEngine *p_vkEngine;
};



