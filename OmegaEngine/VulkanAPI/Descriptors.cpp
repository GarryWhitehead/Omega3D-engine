#include "Descriptors.h"

namespace VulkanAPI
{

DescriptorLayout::DescriptorLayout()
{
}

DescriptorLayout::~DescriptorLayout()
{
}

void DescriptorLayout::add(uint32_t set, uint32_t binding, vk::DescriptorType bindType,
                                 vk::ShaderStageFlags flags)
{
	vk::DescriptorSetLayoutBinding layout(binding, bindType, 1, flags, nullptr);

	layout_bind.layouts[set].push_back(layout);

	// increase count depending on type
	switch (bindType)
	{
	case vk::DescriptorType::eUniformBuffer:
		++layout_bind.uboCount;
		break;
	case vk::DescriptorType::eStorageBuffer:
		++layout_bind.ssboCount;
		break;
	case vk::DescriptorType::eUniformBufferDynamic:
		++layout_bind.uboDynamicCount;
		break;
	case vk::DescriptorType::eStorageBufferDynamic:
		++layout_bind.ssboDynamicCount;
		break;
	case vk::DescriptorType::eCombinedImageSampler:
		++layout_bind.samplerCount;
		break;
	}
}

void DescriptorLayout::prepare(vk::Device dev, const uint32_t imageSets)
{
	// store for destructor
	device = dev;

	// initialise descriptor pool first based on layouts that have been added
	std::vector<vk::DescriptorPoolSize> pools;

	if (layout_bind.uboCount)
	{

		vk::DescriptorPoolSize pool(vk::DescriptorType::eUniformBuffer, layout_bind.uboCount);
		pools.push_back(pool);
	}
	if (layout_bind.samplerCount)
	{

		// we can have multiple sets of images - useful in the case of materials for instance
		vk::DescriptorPoolSize pool(vk::DescriptorType::eCombinedImageSampler, layout_bind.samplerCount * imageSets);
		pools.push_back(pool);
	}
	if (layout_bind.ssboCount)
	{

		vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageBuffer, layout_bind.ssboCount);
		pools.push_back(pool);
	}
	if (layout_bind.uboDynamicCount)
	{

		vk::DescriptorPoolSize pool(vk::DescriptorType::eUniformBufferDynamic, layout_bind.uboDynamicCount);
		pools.push_back(pool);
	}
	if (layout_bind.ssboDynamicCount)
	{

		vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageBufferDynamic, layout_bind.ssboDynamicCount);
		pools.push_back(pool);
	}
	if (layout_bind.storageImageCount)
	{

		vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageImage, layout_bind.storageImageCount);
		pools.push_back(pool);
	}

	assert(!pools.empty());
	uint32_t set_count = static_cast<uint32_t>(layout_bind.layouts.size());
	if (imageSets > 1)
	{
		set_count += imageSets - 1;
	}
	vk::DescriptorPoolCreateInfo createInfo({}, set_count, static_cast<uint32_t>(pools.size()), pools.data());
	VK_CHECK_RESULT(device.createDescriptorPool(&createInfo, nullptr, &pool));

	// and create the descriptor layout for each set
	for (auto set : layout_bind.layouts)
	{
		auto& binding = set.second;
		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(binding.size()), binding.data());

		vk::DescriptorSetLayout layout;
		VK_CHECK_RESULT(device.createDescriptorSetLayout(&layoutInfo, nullptr, &layout));
		descriptorLayouts.push_back(std::make_tuple(set.first, layout));
	}
}

DescriptorSet::DescriptorSet()
{
}

DescriptorSet::~DescriptorSet()
{
}

DescriptorSet::DescriptorSet(vk::Device device, DescriptorLayout& descriptorLayout)
{
	init(device, descriptorLayout);
}

void DescriptorSet::init(vk::Device& device, DescriptorLayout& descriptorLayout)
{
	this->device = device;

	// create all stes that will be reauired - this can be determined by the numbner of layouts we have
	auto& layout = descriptorLayout.getLayout();
	for (uint32_t i = 0; i < layout.size(); ++i)
	{

		uint32_t set = 0;
		vk::DescriptorSetLayout set_layout;
		std::tie(set, set_layout) = layout[i];

		vk::DescriptorSetAllocateInfo allocInfo(descriptorLayout.getDescriptorPool(), 1, &set_layout);

		vk::DescriptorSet descriptorSet;
		VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &descriptorSet));
		descriptorSets[set] = descriptorSet;
	}
}

void DescriptorSet::init(vk::Device& device, DescriptorLayout& descriptorLayout, uint32_t set)
{
	this->device = device;

	vk::DescriptorSetAllocateInfo allocInfo(descriptorLayout.getDescriptorPool(), 1, &descriptorLayout.getLayout(set));

	vk::DescriptorSet descriptorSet;
	VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &descriptorSet));
	descriptorSets[set] = descriptorSet;
}

void DescriptorSet::init(vk::Device& device, vk::DescriptorSetLayout& layout, vk::DescriptorPool& pool, uint32_t set)
{
	this->device = device;

	vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &layout);

	vk::DescriptorSet descriptorSet;
	VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &descriptorSet));
	descriptorSets[set] = descriptorSet;
}

void DescriptorSet::writeSet(ImageReflection::ShaderImageLayout& imageLayout, vk::ImageView& imageView)
{
	vk::DescriptorImageInfo image_info(imageLayout.sampler.getSampler(), imageView, imageLayout.layout);
	vk::WriteDescriptorSet writeSet(descriptorSets[imageLayout.set], imageLayout.binding, 0, 1, imageLayout.type,
	                                &image_info, nullptr, nullptr);
	device.updateDescriptorSets(1, &writeSet, 0, nullptr);
}

void DescriptorSet::writeSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer,
                             uint32_t offset, uint32_t range)
{
	vk::DescriptorBufferInfo buffer_info(buffer, offset, range);
	vk::WriteDescriptorSet writeSet(descriptorSets[set], binding, 0, 1, type, nullptr, &buffer_info, nullptr);
	device.updateDescriptorSets(1, &writeSet, 0, nullptr);
}

void DescriptorSet::writeSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler,
                             vk::ImageView& imageView, vk::ImageLayout layout)
{
	vk::DescriptorImageInfo image_info(sampler, imageView, layout);
	vk::WriteDescriptorSet writeSet(descriptorSets[set], binding, 0, 1, type, &image_info, nullptr, nullptr);
	device.updateDescriptorSets(1, &writeSet, 0, nullptr);
}
}    // namespace VulkanAPI
