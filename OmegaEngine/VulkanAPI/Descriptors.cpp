#include "Descriptors.h"

#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

DescriptorLayout::DescriptorLayout(VkContext& context)
    : context(context)
{
}

DescriptorLayout::~DescriptorLayout()
{
}

void DescriptorLayout::add(uint32_t set, uint32_t binding, vk::DescriptorType bindType, vk::ShaderStageFlags flags)
{
	vk::DescriptorSetLayoutBinding layout(binding, bindType, 1, flags, nullptr);

	bindings.layouts[set].push_back(layout);

	// increase count depending on type
	switch (bindType)
	{
	case vk::DescriptorType::eUniformBuffer:
		++bindings.uboCount;
		break;
	case vk::DescriptorType::eStorageBuffer:
		++bindings.ssboCount;
		break;
	case vk::DescriptorType::eUniformBufferDynamic:
		++bindings.uboDynamicCount;
		break;
	case vk::DescriptorType::eStorageBufferDynamic:
		++bindings.ssboDynamicCount;
		break;
	case vk::DescriptorType::eCombinedImageSampler:
		++bindings.samplerCount;
		break;
	}
}

void DescriptorLayout::prepare()
{
	// initialise descriptor pool first based on layouts that have been added
	std::vector<vk::DescriptorPoolSize> pools;

	if (bindings.uboCount)
	{

		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, bindings.uboCount);
		pools.push_back(poolSize);
	}
	if (bindings.samplerCount)
	{
		// we can have multiple sets of images - useful in the case of materials for instance
		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eCombinedImageSampler, bindings.samplerCount);
		pools.push_back(poolSize);
	}
	if (bindings.ssboCount)
	{

		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBuffer, bindings.ssboCount);
		pools.push_back(poolSize);
	}
	if (bindings.uboDynamicCount)
	{

		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBufferDynamic, bindings.uboDynamicCount);
		pools.push_back(poolSize);
	}
	if (bindings.ssboDynamicCount)
	{

		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBufferDynamic, bindings.ssboDynamicCount);
		pools.push_back(poolSize);
	}
	if (bindings.storageImageCount)
	{

		vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageImage, bindings.storageImageCount);
		pools.push_back(poolSize);
	}

	assert(!pools.empty());
	// creating a pool requires us to the max sets required for this instance.
	// Occassionally, especially in the case of materials, we may need an extra set which might not
	// be added before a call to here. So we add an extra set.
	uint32_t setCount = static_cast<uint32_t>(bindings.layouts.size()) + 1;

	vk::DescriptorPoolCreateInfo createInfo({}, setCount, static_cast<uint32_t>(pools.size()), pools.data());
	VK_CHECK_RESULT(context.getDevice().createDescriptorPool(&createInfo, nullptr, &pool));

	// and create the descriptor layout for each set
	for (auto set : bindings.layouts)
	{
		auto& binding = set.second;
		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(binding.size()), binding.data());

		vk::DescriptorSetLayout layout;
		VK_CHECK_RESULT(context.getDevice().createDescriptorSetLayout(&layoutInfo, nullptr, &layout));
		layouts.emplace(set.first, layout);
	}
}

// ========================== DescriptorSet =================================================
DescriptorSet::DescriptorSet(VkContext& context)
    : context(context)
{
}

DescriptorSet::~DescriptorSet()
{
}


void DescriptorSet::prepare(DescriptorLayout& descrLayout)
{
	// create all descriptor sets that will be required based on the numbner of layouts we have
	for (auto& layout : descrLayout.layouts)
	{
		vk::DescriptorSetAllocateInfo allocInfo(descrLayout.pool, 1, &layout.second);

		vk::DescriptorSet descrSet;
		VK_CHECK_RESULT(context.getDevice().allocateDescriptorSets(&allocInfo, &descrSet));
		// still storing sets based on their set id - this allows them to be quickly referenced for writing
		descrSets[layout.first] = descrSet;
	}
}

void DescriptorSet::updateBufferSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer,
                                    uint32_t offset, uint32_t range)
{
	vk::DescriptorBufferInfo buffer_info(buffer, offset, range);
	vk::WriteDescriptorSet writeSet(descrSets[set], binding, 0, 1, type, nullptr, &buffer_info, nullptr);
	context.getDevice().updateDescriptorSets(1, &writeSet, 0, nullptr);
}

void DescriptorSet::updateImageSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler,
                                   vk::ImageView& imageView, vk::ImageLayout layout)
{
	vk::DescriptorImageInfo image_info(sampler, imageView, layout);
	vk::WriteDescriptorSet writeSet(descrSets[set], binding, 0, 1, type, &image_info, nullptr, nullptr);
	context.getDevice().updateDescriptorSets(1, &writeSet, 0, nullptr);
}
}    // namespace VulkanAPI
