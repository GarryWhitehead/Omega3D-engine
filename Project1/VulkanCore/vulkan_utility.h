#pragma once
#include <vector>
#include <array>
#include "VulkanCore/vulkan_tools.h"
#include "gli.hpp"

class VulkanScene;

struct TextureInfo
{
	TextureInfo() : 
		width(0),
		height(0),
		size(0), 
		mipLevels(1), 
		layers(1), 
		data(nullptr) 
	{}

	uint32_t width;
	uint32_t height;
	uint32_t size;
	uint32_t mipLevels;
	uint32_t layers;
	void *data;
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory texture_mem;
	VkSampler m_tex_sampler;
};

struct BufferData
{
	BufferData() : 
		mappedData(nullptr), 
		size(0)
	{}

	void *mappedData;
	VkBuffer buffer;
	VkDeviceMemory memory;
	uint32_t size;
};

class VulkanUtility
{
public:

	const bool VK_SINGLE_USE = true;
	const bool VK_MULTI_USE = false;
	const bool VK_PRIMARY = true;
	const bool VK_SECONDARY = false;

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

	VulkanUtility();
	VulkanUtility(VulkanScene *vulkanScene);
	~VulkanUtility();

	// Buffer utilities
	VkWriteDescriptorSet InitDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo *info);
	VkWriteDescriptorSet InitDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo *info);
	VkDescriptorImageInfo InitImageInfoDescriptor(VkImageLayout layout, VkImageView view, VkSampler sampler);
	VkDescriptorBufferInfo InitBufferInfoDescriptor(VkBuffer buffer, int offset, uint32_t range);
	VkDescriptorSetLayoutBinding InitLayoutBinding(int binding, VkDescriptorType type, VkShaderStageFlags flags);
	void CreateBuffer(uint32_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& devMemory);
	uint32_t FindMemoryType(uint32_t type, VkMemoryPropertyFlags flags);
	void CopyBuffer(VkBuffer dest, VkBuffer src, VkDeviceSize size, VkCommandPool cmdPool);

	template <typename T>
	void MapBuffer(BufferData buffer, std::vector<T>& data);

	// command buffer utilities
	std::vector<VkFramebuffer> InitFrameBuffers(uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView imageView);
	VkCommandPool InitCommandPool(uint32_t index);
	VkCommandBuffer CreateCmdBuffer(bool primary, bool singleUse, VkFramebuffer frameBuffer, VkRenderPass renderPass, VkCommandPool cmdPool);
	VkCommandBuffer CreateTempCmdBuffer(VkCommandPool cmdPool);
	void EndCmdBuffer(VkCommandBuffer cmdBuffer, VkCommandPool cmdPool, VkQueue queue);
	bool CheckForCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer);
	void DestroyCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer, VkCommandPool cmdPool);

	// pipeline utilites
	VkPipelineViewportStateCreateInfo InitViewPort(VkViewport& viewPort, VkRect2D& scissor, float width, float height);
	VkPipelineRasterizationStateCreateInfo InitRasterzationState(VkPolygonMode polyMode, VkCullModeFlagBits cullMode, VkFrontFace frontFace);
	VkPipelineMultisampleStateCreateInfo InitMultisampleState(VkSampleCountFlagBits flag);

	// shader utilites
	VkPipelineShaderStageCreateInfo InitShaders(std::string shaderFile, VkShaderStageFlagBits stage);
	void LoadFile(std::string filename, std::vector<char>& data);
	VkShaderModule CreateShaderModule(std::vector<char>& shader);
	VkPipelineShaderStageCreateInfo CreateShader(VkShaderModule shader, VkShaderStageFlagBits stage);

	// texture
	TextureInfo LoadTexture(std::string filename, VkSamplerAddressMode addrMode, VkCompareOp compare, float maxAnisotropy, VkBorderColor color, VkFormat format, VkCommandPool cmdPool);
	TextureInfo LoadTextureArray(std::string filename, VkSamplerAddressMode addrMode, VkCompareOp compare, float maxAnisotropy, VkBorderColor color, VkFormat format, VkCommandPool cmdPool);
	void ImageTransition(VkCommandBuffer cmdBuff, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout,uint32_t mipLevels, uint32_t layers, VkCommandPool cmdPool);
	void CreateTextureSampler(TextureInfo &texture, VkSamplerAddressMode addressMode, float maxAnisotropy, VkCompareOp compareOp, VkBorderColor borderColor);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& requiredFormats, VkImageTiling tiling, VkFormatFeatureFlags features);

	// render
	uint32_t InitRenderFrame();
	void SubmitFrame(uint32_t imageIndex);

protected:

	VulkanScene *p_vulkanScene;
};

template <typename T>
void VulkanUtility::MapBuffer(BufferData buffer, std::vector<T>& bufferData)
{
	assert(buffer.size != 0);
	if (buffer.mappedData == nullptr) {
		VK_CHECK_RESULT(vkMapMemory(p_vulkanScene->m_device.device, buffer.memory, 0, VK_WHOLE_SIZE, 0, &buffer.mappedData));
		memcpy(buffer.mappedData, bufferData.data(), bufferData.size() * sizeof(T));
		vkUnmapMemory(p_vulkanScene->m_device.device, buffer.memory);
	}
	else {
		memcpy(buffer.mappedData, bufferData.data(), bufferData.size() * sizeof(T));
	}
}


