#pragma once
#include "VulkanCore/vulkan_utility.h"

class VulkanEngine;
class VkMemoryManager;

class VulkanTexture
{
public:

	VulkanTexture();
	~VulkanTexture();
	
	void Destroy(VkDevice device);
	void PrepareImage(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, VulkanEngine *vkEngine, float maxAnisotropy = 1.0f, bool createSampler = true);
	void PrepareImageArray(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, uint32_t mipLevels, uint32_t layers, VulkanEngine *vkEngine, bool isCube = false);
	void UploadDataToImage(void* tex_data, uint32_t size, VkMemoryManager *p_vkMemory, VulkanEngine *p_vkEngine);
	void LoadTexture(std::string filename, const VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VulkanEngine *p_vkEngine, VkMemoryManager *p_vkMemory);
	void LoadTextureArray(std::string filename, VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VulkanEngine *p_vkEngine, VkMemoryManager *p_vkMemory);
	void LoadCubeMap(std::string filename, const VkFormat format, const VkCommandPool cmdPool, VulkanEngine *p_vkEngine, VkMemoryManager *p_vkMemory);
	void CreateTextureSampler(const VkSamplerAddressMode addressMode, float maxAnisotropy, const VkBorderColor borderColor, VulkanEngine *p_vkEngine);

	// texture dimensions
	uint32_t width;
	uint32_t height;
	uint32_t size;
	uint32_t mipLevels;
	uint32_t layers;
	void *data;

	// Vulkan related 
	VkDeviceMemory texMemory;
	VkImage image;
	VkFormat format;
	VkImageView imageView;
	VkSampler texSampler;

};

