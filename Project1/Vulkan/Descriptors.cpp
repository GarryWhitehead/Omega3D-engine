#include "Descriptors.h"

namespace VulkanAPI
{

	DescriptorLayout::DescriptorLayout()
	{

	}

	DescriptorLayout::~DescriptorLayout()
	{
	}

	void DescriptorLayout::add_layout(uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags)
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

	void DescriptorLayout::create(vk::Device dev)
	{
		// store for destructor
		device = dev;

		// initialise descriptor pool first based on layouts that have been added
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

		// and create the descriptor layout
		vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(layout_bind.layouts.size()), layout_bind.layouts.data());
		VK_CHECK_RESULT(device.createDescriptorSetLayout(&layoutInfo, nullptr, &layout));
	}

	DescriptorSet::DescriptorSet(vk::Device device, DescriptorLayout descr_layout)
	{
		vk::DescriptorSetAllocateInfo allocInfo(descr_layout.get_pool(), 1, &descr_layout.get_layout());
		VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &set));
	}

	void DescriptorSet::update_set(uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, uint32_t offset, uint32_t range)
	{
		vk::DescriptorBufferInfo buffer_info(buffer, offset, range);
		vk::WriteDescriptorSet write_set(set, binding, 0, 1, type, nullptr, &buffer_info, nullptr);
	}

	void DescriptorSet::update_set(uint32_t binding, vk::DescriptorType type, vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout layout)
	{
		vk::DescriptorImageInfo image_info(sampler, image_view, layout);
		vk::WriteDescriptorSet write_set(set, binding, 0, 1, type, &image_info, nullptr, nullptr);
	}

	void DescriptorSet::update(vk::Device device)
	{
		assert(!write_sets.empty());
		device.updateDescriptorSets(static_cast<uint32_t>(write_sets.size()), write_sets.data(), 0, nullptr);
	}
}
