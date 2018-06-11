#pragma once
#include "VulkanCore/vulkan_tools.h"

class VkDescriptors
{

public:

	// simplified data sets for vulkan descriptors
	struct LayoutBinding
	{
		VkDescriptorType type; 
		VkShaderStageFlags flags;
	};


	VkDescriptors();
	~VkDescriptors();

	void AddDescriptorBindings(std::vector<LayoutBinding>& bindings);
	void GenerateDescriptorSets(VkDescriptorBufferInfo *bufferInfo, VkDescriptorImageInfo *imageInfo, VkDevice device);
	void Destroy(VkDevice device);

	VkDescriptorSet set;
	VkDescriptorSetLayout layout;
	VkDescriptorPool pool;

private:

	void PrepareDescriptorPools(std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDevice device);

	// layout bindings data
	std::vector<VkDescriptorSetLayoutBinding> layouts;

};

