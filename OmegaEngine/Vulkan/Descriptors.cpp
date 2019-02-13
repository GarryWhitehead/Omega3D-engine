#include "Descriptors.h"

namespace VulkanAPI
{

	DescriptorLayout::DescriptorLayout()
	{

	}

	DescriptorLayout::~DescriptorLayout()
	{
	}

	void DescriptorLayout::add_layout(uint32_t set, uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags)
	{
		vk::DescriptorSetLayoutBinding layout(
			binding, 
			bind_type, 1, flags, nullptr);

		layout_bind[set].push_back(layout);

		// increase count depending on type
		switch (bind_type) {
		case vk::DescriptorType::eUniformBuffer:
			++layout_bind.ubo_count;
			break;
		case vk::DescriptorType::eStorageBuffer:
			++layout_bind.ssbo_count;
			break;
		case vk::DescriptorType::eUniformBufferDynamic:
			++layout_bind.ubo_dynamic_count;
			break;
		case vk::DescriptorType::eStorageBufferDynamic:
			++layout_bind.ssbo_dynamic_count;
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
		if (layout_bind.ubo_dynamic_count) {

			vk::DescriptorPoolSize pool(vk::DescriptorType::eUniformBufferDynamic, layout_bind.ubo_dynamic_count);
			pools.push_back(pool);
		}
		if (layout_bind.ssbo_dynamic_count) {

			vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageBufferDynamic, layout_bind.ssbo_dynamic_count);
			pools.push_back(pool);
		}
		if (layout_bind.storage_image_count) {
			
			vk::DescriptorPoolSize pool(vk::DescriptorType::eStorageImage, layout_bind.storage_image_count);
			pools.push_back(pool);
		}

		assert(!pools.empty());
		vk::DescriptorPoolCreateInfo createInfo({}, 1, static_cast<uint32_t>(pools.size()), pools.data());
		VK_CHECK_RESULT(device.createDescriptorPool(&createInfo, nullptr, &pool));

		// and create the descriptor layout for each set
		for (auto set : layouts) {
			auto& layout_bind = set.second;
			vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(layout_bind.layouts.size()), layout_bind.layouts.data());
			
			vk::DescriptorSetLayout layout;
			VK_CHECK_RESULT(device.createDescriptorSetLayout(&layoutInfo, nullptr, &layout));
			descr_layouts.push_back(std::make_tuple(set.first, layout));
		}
	}

	DescriptorSet::DescriptorSet()
	{

	}

	DescriptorSet::~DescriptorSet()
	{

	}

	DescriptorSet::DescriptorSet(vk::Device device, DescriptorLayout descr_layout)
	{
		init(device, descr_layout);
	}

	void DescriptorSet::init(vk::Device device, DescriptorLayout descr_layout)
	{
		// create all stes that will be reauired - this can be determined by the numbner of layouts we have
		for (uint32_t i = 0; i < descr_layout.descr_layouts.size(); ++i) {

			vk::DescriptorSetAllocateInfo allocInfo(descr_layout.get_pool(), i, &descr_layout.get_layout());

			vk::DescriptorSet set;
			VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &set));
			descr_sets.push_back(set);
		}
	}

	void DescriptorSet::write_set(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, uint32_t offset, uint32_t range)
	{
		vk::DescriptorBufferInfo buffer_info(buffer, offset, range);
		vk::WriteDescriptorSet write_set(descr_sets[set], binding, 0, 1, type, nullptr, &buffer_info, nullptr);
		write_sets[set].push_back(write_set);
	}

	void DescriptorSet::uwrite_set(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout layout)
	{
		vk::DescriptorImageInfo image_info(sampler, image_view, layout);
		vk::WriteDescriptorSet write_set(decsr_sets[set], binding, 0, 1, type, &image_info, nullptr, nullptr);
		write_sets[set].push_back(write_set);
	}

	void DescriptorSet::update_sets(vk::Device device)
	{
		// update descriptors for all sets
		for (auto set : write_sets) {

			assert(!set.second.empty());
			device.updateDescriptorSets(static_cast<uint32_t>(set.second.size()), set.second.data(), 0, nullptr);
		}
	}
}
 