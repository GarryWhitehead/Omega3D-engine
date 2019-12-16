#include "Descriptors.h"

#include "VulkanAPI/VkContext.h"

namespace VulkanAPI
{

DescriptorPool::DescriptorPool(VkContext& context)
    : context(context)
{
}

DescriptorPool::~DescriptorPool()
{
	context.getDevice().destroy(pool, nullptr);
}

DescriptorLayout& DescriptorPool::createLayout(uint32_t set, uint32_t binding, vk::DescriptorType bindType,
                                               vk::ShaderStageFlags flags)
{
	DescriptorLayout layout(context, *this);
	layout.set = set;
	layout.layoutBinding = vk::DescriptorSetLayoutBinding{ binding, bindType, 1, flags, nullptr };

	layouts.emplace_back(layout);

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

	return layouts.back();
}

void DescriptorPool::build()
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
}

// ========================= DescriptorLayout ================================================
DescriptorLayout::DescriptorLayout(VkContext& context, DescriptorPool& pool)
    : context(context)
    , pool(pool)
{
}

DescriptorLayout::~DescriptorLayout()
{
	if (descrlayout)
	{
		context.getDevice().destroy(descrlayout, nullptr);
	}
}

void DescriptorLayout::prepare()
{
	vk::DescriptorSetLayoutCreateInfo layoutInfo({}, 1, &layoutBinding);
	VK_CHECK_RESULT(context.getDevice().createDescriptorSetLayout(&layoutInfo, nullptr, &descrlayout));
}

// ========================== DescriptorSet =================================================
DescriptorSet::DescriptorSet(VkContext& context)
    : context(context)
{
}

DescriptorSet::~DescriptorSet()
{
}

void DescriptorSet::addLayout(DescriptorLayout& layout)
{
	vk::DescriptorSetAllocateInfo allocInfo(layout.pool.get(), 1, &layout.descrlayout);

	vk::DescriptorSet descrSet;
	VK_CHECK_RESULT(context.getDevice().allocateDescriptorSets(&allocInfo, &descrSet));

	// still storing sets based on their set id - this allows them to be quickly referenced for writing
	descrSets[layout.set] = descrSet;
}

void DescriptorSet::updateBufferSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer,
                                    uint32_t offset, uint32_t range)
{
	auto iter = descrSets.find(set);
	assert(iter != descrSets.end());

	vk::DescriptorBufferInfo buffer_info(buffer, offset, range);
	vk::WriteDescriptorSet writeSet(iter->second, binding, 0, 1, type, nullptr, &buffer_info, nullptr);
	context.getDevice().updateDescriptorSets(1, &writeSet, 0, nullptr);
}

void DescriptorSet::updateImageSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler,
                                   vk::ImageView& imageView, vk::ImageLayout layout)
{
	auto iter = descrSets.find(set);
	assert(iter != descrSets.end());

	vk::DescriptorImageInfo image_info(sampler, imageView, layout);
	vk::WriteDescriptorSet writeSet(iter->second, binding, 0, 1, type, &image_info, nullptr, nullptr);
	context.getDevice().updateDescriptorSets(1, &writeSet, 0, nullptr);
}
}    // namespace VulkanAPI
