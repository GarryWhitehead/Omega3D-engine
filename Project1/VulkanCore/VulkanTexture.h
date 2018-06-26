#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

class VkMemoryManager;

class VulkanTexture
{

public:

	VulkanTexture(VkPhysicalDevice p_dev, VkDevice dev);
	~VulkanTexture();
	
	void Destroy();
	void PrepareImage(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, float maxAnisotropy = 1.0f, bool createSampler = true, uint32_t mipLevels = 1, VkFilter filter = VK_FILTER_LINEAR);
	void PrepareImageArray(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, uint32_t mipLevels, uint32_t layers, bool isCube = false);
	void UploadDataToImage(void* tex_data, uint32_t size, VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory);
	void GenerateMipChain(uint32_t mipLevels, VkCommandPool cmdPool, VkQueue graphQueue);
	void LoadTexture(std::string filename, const VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory);
	void LoadTextureArray(std::string filename, VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory);
	void LoadCubeMap(std::string filename, const VkFormat format, const VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory);
	void CreateTextureSampler(const VkSamplerAddressMode addressMode, float maxAnisotropy, const VkBorderColor borderColor, VkFilter filter);

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

private:

	VkPhysicalDevice physDevice;
	VkDevice device;

};

