#include "Descriptors.h"
#include "Vulkan/Shader.h"

namespace VulkanAPI
{
	namespace Util
	{
		// a static version or the write set function, useful for updating a single descriptor set
		void write_set(vk::Device device, vk::DescriptorSet& descr_set, uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler, vk::ImageView& image_view, vk::ImageLayout layout)
		{
			vk::DescriptorImageInfo image_info(sampler, image_view, layout);
			vk::WriteDescriptorSet write_set(descr_set, binding, 0, 1, type, &image_info, nullptr, nullptr);
			device.updateDescriptorSets(1, &write_set, 0, nullptr);
		}
	}

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

		layout_bind.layouts[set].push_back(layout);

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
		case vk::DescriptorType::eCombinedImageSampler:
			++layout_bind.sampler_count;
			break;
		}
	}

	void DescriptorLayout::create(vk::Device dev, const uint32_t image_sets)
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

			// we can have multiple sets of images - useful in the case of materials for instance
			vk::DescriptorPoolSize pool(vk::DescriptorType::eCombinedImageSampler, layout_bind.sampler_count * image_sets);
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
		uint32_t set_count = static_cast<uint32_t>(layout_bind.layouts.size());
		if (image_sets > 1) {
			set_count += image_sets - 1;
		}
		vk::DescriptorPoolCreateInfo createInfo({}, set_count, static_cast<uint32_t>(pools.size()), pools.data());
		VK_CHECK_RESULT(device.createDescriptorPool(&createInfo, nullptr, &pool));

		// and create the descriptor layout for each set
		for (auto set : layout_bind.layouts) {
			auto& binding = set.second;
			vk::DescriptorSetLayoutCreateInfo layoutInfo({}, static_cast<uint32_t>(binding.size()), binding.data());
			
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

	DescriptorSet::DescriptorSet(vk::Device device, DescriptorLayout descr_layout, uint32_t set)
	{
		init(device, descr_layout, set);
	}

	void DescriptorSet::init(vk::Device& device, DescriptorLayout& descr_layout)
	{
		this->device = device;

		// create all stes that will be reauired - this can be determined by the numbner of layouts we have
		auto& layout = descr_layout.get_layout();
		for (uint32_t i = 0; i < layout.size(); ++i) {

			uint32_t set = 0;	
			vk::DescriptorSetLayout set_layout;
			std::tie(set, set_layout) = layout[i];

			vk::DescriptorSetAllocateInfo allocInfo(descr_layout.get_pool(), 1, &set_layout);

			vk::DescriptorSet descr_set;
			VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &descr_set));
			descr_sets[set] = descr_set;
		}
	}

	void DescriptorSet::init(vk::Device& device, DescriptorLayout& descr_layout, uint32_t set)
	{
		this->device = device;

		vk::DescriptorSetAllocateInfo allocInfo(descr_layout.get_pool(), 1, &descr_layout.get_layout(set));

		vk::DescriptorSet descr_set;
		VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &descr_set));
		descr_sets[set] = descr_set;
	}

	void DescriptorSet::init(vk::Device& device, vk::DescriptorSetLayout& layout, vk::DescriptorPool& pool, uint32_t set)
	{
		this->device = device;

		vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &layout);

		vk::DescriptorSet descr_set;
		VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &descr_set));
		descr_sets[set] = descr_set;
	}

	void DescriptorSet::write_set(ShaderImageLayout& imageLayout, vk::ImageView& image_view)
	{
		vk::DescriptorImageInfo image_info(imageLayout.sampler.get_sampler(), image_view, imageLayout.layout);
		vk::WriteDescriptorSet write_set(descr_sets[imageLayout.set], imageLayout.binding, 0, 1, imageLayout.type, &image_info, nullptr, nullptr);
		device.updateDescriptorSets(1, &write_set, 0, nullptr);
	}

	void DescriptorSet::write_set(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, uint32_t offset, uint32_t range)
	{
		vk::DescriptorBufferInfo buffer_info(buffer, offset, range);
		vk::WriteDescriptorSet write_set(descr_sets[set], binding, 0, 1, type, nullptr, &buffer_info, nullptr);
		device.updateDescriptorSets(1, &write_set, 0, nullptr);

	}

	void DescriptorSet::write_set(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler, vk::ImageView& image_view, vk::ImageLayout layout)
	{
		vk::DescriptorImageInfo image_info(sampler, image_view, layout);
		vk::WriteDescriptorSet write_set(descr_sets[set], binding, 0, 1, type, &image_info, nullptr, nullptr);
		device.updateDescriptorSets(1, &write_set, 0, nullptr);
	}
}
 