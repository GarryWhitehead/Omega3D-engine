#pragma once
#include "Vulkan/Common.h"

namespace VulkanAPI
{

	class Descriptors
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

		Descriptors(vk::Device device);
		~Descriptors();

		void add_layout(uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags);
		void create_descriptor_pools();
		void add_descriptor_set(uint32_t set, uint32_t binding, vk::DescriptorType bind_type);

	private:

		vk::Device device;

		vk::DescriptorPool pool;
		VkDescriptorSetAllocateInfo info;
		std::vector<vk::WriteDescriptorSet> sets;
		LayoutBindings layout_bind;
	};

}

