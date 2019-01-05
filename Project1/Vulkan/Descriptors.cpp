#include "Descriptors.h"

namespace VulkanAPI
{

	Descriptors::Descriptors(vk::Device dev) :
		device(dev)
	{
	}


	Descriptors::~Descriptors()
	{
	}

	void Descriptors::add_layout(uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags)
	{
		vk::DescriptorSetLayoutBinding layout(
			binding, 
			bind_type, 1, flags, nullptr);

		layout_bind.layouts.push_back(layout);

		// increase count depending on type
		switch (bind_type) {
		case vk::DescriptorType::eUniformBuffer:
			++layout_bind.ubo_count;
			break;
		case vk::DescriptorType::eStorageBuffer:
			++layout_bind.ssbo_count;
			break;
		case vk::DescriptorType::eSampler:
			++layout_bind.sampler_count;
			break;
		}
	}

	void Descriptors::create_descriptor_pools()
	{
		std::vector<vk::DescriptorPoolSize> pools;

		if (layout_bind.ubo_count) {

			vk::DescriptorPoolSize pool(vk::DescriptorType::eUniformBuffer, layout_bind.ubo_count);
			pools.push_back(pool);
		}
		if (layout_bind.sampler_count) {

			vk::DescriptorPoolSize pool(vk::DescriptorType::eSampler, layout_bind.sampler_count);
			pools.push_back(pool);
		}
		if (layout_bind.ssbo_count) {

			vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageBuffer, layout_bind.ssbo_count);
			pools.push_back(pool);
		}
		if (layout_bind.storage_image_count) {
			
			vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageImage, layout_bind.storage_image_count);
			pools.push_back(pool);
		}

		assert(!pools.empty());
		vk::DescriptorPoolCreateInfo createInfo({}, 1, static_cast<uint32_t>(pools.size()), pools.data());

		VK_CHECK_RESULT(device.createDescriptorPool(&createInfo, nullptr, &pool));
	}

	void Descriptors::add_descriptor_set(uint32_t set, uint32_t binding, vk::DescriptorType bind_type)
	{
		vk::WriteDescriptorSet descrSet = {};
		descrSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descrSet.dstSet = set;
		descrSet.dstBinding = c;
		descrSet.dstArrayElement = 0;
		descrSet.descriptorCount = 1;
		descrSet.descriptorType = layouts[c].descriptorType;
	}
}
