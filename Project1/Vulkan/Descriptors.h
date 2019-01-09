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
			std::unordered_map<vk::DescriptorType, vk::DescriptorSetLayoutBinding> layouts;

			// running counts of each descriptor type - required for creating descriptor pools
			uint32_t ubo_count = 0;
			uint32_t ssbo_count = 0;
			uint32_t sampler_count = 0;
			uint32_t storage_image_count = 0;
		};

		DescriptorLayout(vk::Device device);
		~DescriptorLayout();

		void add_layout(uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags);
		void create();
		

	private:

		vk::Device device;

		vk::DescriptorPool pool;
		vk::DescriptorSetLayout layout;
		LayoutBindings layout_bind;
	};

	class DescriptorSet
	{

	public:
		
		DescriptorSet(vk::Device device, vk::DescriptorPool& pool, vk::DescriptorSetLayout& layout);

		void update_set(uint32_t binding, vk::DescriptorType type, vk::Buffer buffer, uint32_t offset, uint32_t range);
		void update_set(uint32_t binding, vk::DescriptorType type, vk::Sampler sampler, vk::ImageView image_view, vk::ImageLayout layout);
		void update(vk::Device device);

	private:

		vk::DescriptorSet set;

		std::vector<vk::WriteDescriptorSet> write_sets;
	};

}

