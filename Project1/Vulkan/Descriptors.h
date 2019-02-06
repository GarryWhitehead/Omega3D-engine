#pragma once
#include "Vulkan/Common.h"
#include <unordered_map>

namespace VulkanAPI
{

	class DescriptorLayout
	{

	public:

		struct LayoutBindings
		{
			std::vector<vk::DescriptorSetLayoutBinding> layouts;

			// running counts of each descriptor type - required for creating descriptor pools
			uint32_t ubo_count = 0;
			uint32_t ssbo_count = 0;
			uint32_t sampler_count = 0;
			uint32_t storage_image_count = 0;
		};

		DescriptorLayout();
		~DescriptorLayout();

		void add_layout(uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags);
		void create(vk::Device device);
		
		vk::DescriptorSetLayout& get_layout()
		{
			assert(layout);
			return layout;
		}

		vk::DescriptorPool& get_pool()
		{
			assert(pool);
			return pool;
		}

	private:

		vk::Device device;

		vk::DescriptorPool pool;
		vk::DescriptorSetLayout layout;
		LayoutBindings layout_bind;
	};

	class DescriptorSet
	{

	public:
		
		DescriptorSet();
		~DescriptorSet();
		DescriptorSet(vk::Device device, DescriptorLayout descr_layout);

		void init(vk::Device device, DescriptorLayout descr_layout);
		void update_set(uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, uint32_t offset, uint32_t range);
		void update_set(uint32_t binding, vk::DescriptorType type, vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout layout);
		void update(vk::Device device);

		vk::DescriptorSet& get()
		{
			assert(set);
			return set;
		}

	private:

		vk::DescriptorSet set;

		std::vector<vk::WriteDescriptorSet> write_sets;
	};

}

