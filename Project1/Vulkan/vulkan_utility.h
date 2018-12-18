#pragma once
#include <vector>
#include <array>
#include "VulkanCore/vulkan_tools.h"
#include "gli.hpp"


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

	VulkanUtility();
	~VulkanUtility();

	static uint32_t FindMemoryType(const uint32_t type, const VkMemoryPropertyFlags flags, VkPhysicalDevice physDevice);
	static void CopyBuffer(const VkBuffer dest, const VkBuffer src, const VkDeviceSize size, const VkCommandPool cmdPool, VkQueue graphQueue, VkDevice device, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	// command buffer utilities
	static VkCommandPool InitCommandPool(const uint32_t index, VkDevice device);
	static VkCommandBuffer CreateCmdBuffer(bool primary, bool singleUse, const VkFramebuffer frameBuffer, const VkRenderPass renderPass, const VkCommandPool cmdPool, VkDevice device);
	static void SubmitCmdBufferToQueue(const VkCommandBuffer cmdBuffer, const VkQueue queue, const VkCommandPool cmdPool, VkDevice device);
	static bool CheckForCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer);

	// pipeline utilites
	static VkPipelineViewportStateCreateInfo InitViewPortCreateInfo(VkViewport& viewPort, VkRect2D& scissor, const uint32_t width, const uint32_t height);
	static VkViewport InitViewPort(const uint32_t width, const uint32_t height, const float minDepth, const float maxDepth);
	static VkRect2D InitScissor(const uint32_t width, const uint32_t height, const uint32_t x, const uint32_t y);
	static VkPipelineRasterizationStateCreateInfo InitRasterzationState(const VkPolygonMode polyMode, const VkCullModeFlagBits cullMode, const VkFrontFace frontFace);
	static VkPipelineMultisampleStateCreateInfo InitMultisampleState(const VkSampleCountFlagBits flag);

	// shader utilites
	static VkPipelineShaderStageCreateInfo InitShaders(std::string shaderFile, const VkShaderStageFlagBits stage, VkDevice device);
	static void LoadFile(std::string filename, std::vector<char>& data);
	static VkShaderModule CreateShaderModule(std::vector<char>& shader, VkDevice device);
	static VkPipelineShaderStageCreateInfo CreateShader(const VkShaderModule shader, const VkShaderStageFlagBits stage);

	// render
	static uint32_t InitRenderFrame(VkDevice device, VkSwapchainKHR swapChain, VkSemaphore renderSem);
	static void SubmitFrame(const uint32_t imageIndex, VkSwapchainKHR swapChain, VkSemaphore renderSem, VkQueue presentQueue);

	// image
	static void ImageTransition(const VkQueue graphQueue, const VkCommandBuffer cmdBuff, const VkImage image, const VkFormat format, const VkImageLayout old_layout, const VkImageLayout new_layout, const VkCommandPool cmdPool, VkDevice device, uint32_t levelCount = 1, uint32_t layers = 1, uint32_t mipLevel = 0);
	static VkFormat FindSupportedFormat(const std::vector<VkFormat>& requiredFormats, const VkImageTiling tiling, const VkFormatFeatureFlags features, VkPhysicalDevice physDevice);
	static VkImageView InitImageView(VkImage image, VkFormat format, VkImageAspectFlagBits imageAspect, VkImageViewType type, VkDevice device);

	// barrier tools
	static VkSemaphore CreateSemaphore(VkDevice device);
	static VkFence CreateFence(VkFenceCreateFlags flags, VkDevice device);

};



