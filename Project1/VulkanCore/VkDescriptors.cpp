#include "VkDescriptors.h"



VkDescriptors::VkDescriptors()
{
}


VkDescriptors::~VkDescriptors()
{
}

void VkDescriptors::PrepareDescriptorPools(std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDevice device)
{
	assert(!bindings.empty());
	
	// generate descriptr pools depending on binding types
	uint32_t uniform_count = 0;
	uint32_t combined_count = 0;
	uint32_t storage_buffer_count = 0;
	uint32_t storage_image_count = 0;
	uint32_t input_count = 0;

	for (auto& bind : bindings) {

		if (bind.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			++uniform_count;
		else if (bind.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			++combined_count;
		else if (bind.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
			++input_count;
		else if (bind.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			++storage_buffer_count;
		else if (bind.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			++storage_image_count;
	}

	//create pools depedning on type counts
	std::vector<VkDescriptorPoolSize> poolSizes = {};

	if (uniform_count) {

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = uniform_count;
		poolSizes.push_back(poolSize);
	}
	if (combined_count) {

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = combined_count;
		poolSizes.push_back(poolSize);
	}
	if (input_count) {

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		poolSize.descriptorCount = input_count;
		poolSizes.push_back(poolSize);
	}
	if (storage_buffer_count) {

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSize.descriptorCount = storage_buffer_count;
		poolSizes.push_back(poolSize);
	}
	if (storage_image_count) {

		VkDescriptorPoolSize poolSize = {};
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSize.descriptorCount = storage_image_count;
		poolSizes.push_back(poolSize);
	}

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &createInfo, nullptr, &pool));
}

void VkDescriptors::AddDescriptorBindings(std::vector<LayoutBinding>& bindings)
{
	// init the layout bindings
	assert(!bindings.empty());
	
	for (uint32_t c = 0; c < bindings.size(); ++c) {

		VkDescriptorSetLayoutBinding layout = {};
		layout.binding = c;
		layout.descriptorCount = 1;
		layout.descriptorType = bindings[c].type;
		layout.pImmutableSamplers = nullptr;
		layout.stageFlags = bindings[c].flags;

		layouts.push_back(layout);
	}
}

void VkDescriptors::GenerateDescriptorSets(VkDescriptorBufferInfo *bufferInfo, VkDescriptorImageInfo *imageInfo, VkDevice device)
{
	// make sure the user has defined the layouts and descriptor info
	assert(!layouts.empty());
	
	// create the descriptor pools
	PrepareDescriptorPools(layouts, device);

	// create layouts
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layouts.size());
	layoutInfo.pBindings = layouts.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

	// init descriptor sets
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &set));

	// update sets with descriptor info
	std::vector<VkWriteDescriptorSet> writeSets;

	for (uint32_t c = 0; c < layouts.size(); ++c) {

		VkWriteDescriptorSet descrSet = {};
		descrSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descrSet.dstSet = set;
		descrSet.dstBinding = c;
		descrSet.dstArrayElement = 0;
		descrSet.descriptorCount = 1;
		descrSet.descriptorType = layouts[c].descriptorType;

		if (layouts[c].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || layouts[c].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {

			descrSet.pBufferInfo = bufferInfo;	
			bufferInfo++;
		}
		else if (layouts[c].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || layouts[c].descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT || layouts[c].descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) {

			descrSet.pImageInfo = imageInfo;
			imageInfo++;
		}

		writeSets.push_back(descrSet);
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeSets.size()), writeSets.data(), 0, nullptr);
}

void VkDescriptors::Destroy(VkDevice device)
{
	vkDestroyDescriptorSetLayout(device, layout, nullptr);
	vkDestroyDescriptorPool(device, pool, nullptr);

	set = VK_NULL_HANDLE;
}
