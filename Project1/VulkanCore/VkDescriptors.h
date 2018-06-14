#pragma once
#include "VulkanCore/vulkan_tools.h"
#include <vector>

class VkDescriptors
{

public:

	// simplified data sets for vulkan descriptors
	struct LayoutBinding
	{
		VkDescriptorType type; 
		VkShaderStageFlags flags;
	};


	VkDescriptors(VkDevice dev);
	~VkDescriptors();

	void AddDescriptorBindings(std::vector<LayoutBinding>& bindings);
	void GenerateDescriptorSets(VkDescriptorBufferInfo *bufferInfo, VkDescriptorImageInfo *imageInfo);
	void Destroy();

	VkDescriptorSet set;
	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;

private:

	void PrepareDescriptorPools(std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDevice device);

	// layout bindings data
	std::vector<VkDescriptorSetLayoutBinding> layouts;

	VkDevice device;

};

