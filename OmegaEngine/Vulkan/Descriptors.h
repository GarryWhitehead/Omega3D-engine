#pragma once
#include "Vulkan/Common.h"
#include <unordered_map>
#include <tuple>

namespace VulkanAPI
{
	namespace Util
	{
		void write_set(vk::Device device, vk::DescriptorSet& descr_set, uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler, vk::ImageView& image_view, vk::ImageLayout layout);
	}

	struct ShaderImageLayout;

	class DescriptorLayout
	{

	public:

		struct LayoutBindings
		{
			// grouped into sets 
			std::unordered_map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding> > layouts;

			// running counts of each descriptor type - required for creating descriptor pools
			// using one pool for multiple sets
			uint32_t ubo_count = 0;
			uint32_t ssbo_count = 0;
			uint32_t sampler_count = 0;
			uint32_t ubo_dynamic_count = 0;
			uint32_t ssbo_dynamic_count = 0;
			uint32_t storage_image_count = 0;
		};

		DescriptorLayout();
		~DescriptorLayout();

		void add_layout(uint32_t set, uint32_t binding, vk::DescriptorType bind_type, vk::ShaderStageFlags flags);
		void create(vk::Device device);
		
		std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout> >& get_layout()
		{
			assert(!descr_layouts.empty());
			return descr_layouts;
		}

		vk::DescriptorSetLayout& get_layout(uint32_t set)
		{
			for (auto& layout : descr_layouts) {
				if (std::get<0>(layout) == set) {
					return std::get<1>(layout);
				}
			}
			throw std::runtime_error("Unable to find descriptor layout.");
		}

		vk::DescriptorPool& get_pool()
		{
			assert(pool);
			return pool;
		}

	private:

		vk::Device device;

		vk::DescriptorPool pool;

		// each set will need it's own layout - keep track of the set number so we can match them correctly later
		std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout> > descr_layouts;
		LayoutBindings layout_bind;
	};

	class DescriptorSet
	{

	public:
		
		DescriptorSet();
		~DescriptorSet();
		DescriptorSet(vk::Device device, DescriptorLayout descr_layout);

		void init(vk::Device device, DescriptorLayout descr_layout);
		void init(vk::Device device, vk::DescriptorSetLayout layout, vk::DescriptorPool& pool, uint32_t set);

		void write_set(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, uint32_t offset, uint32_t range);
		void write_set(ShaderImageLayout& imageLayout, vk::ImageView& image_view);

		// use this when you haven't reflected the shader
		void write_set(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler, vk::ImageView& image_view, vk::ImageLayout layout);

		vk::DescriptorSet& get(uint32_t set)
		{
			assert(!descr_sets.empty());
			return descr_sets[set];
		}

		std::vector<vk::DescriptorSet> get()
		{
			assert(!descr_sets.empty());
			// first get the bindings
			std::vector<uint32_t> bindings;
			for (auto& set : descr_sets) {
				bindings.push_back(set.first);
			}

			// sort into ascending order
			std::sort(bindings.begin(), bindings.end());

			// now create the sets
			std::vector< vk::DescriptorSet> sets;
			for (auto& bind : bindings) {
				sets.push_back(descr_sets[bind]);
			}
			return sets;
		}

		uint32_t get_size() const
		{
			if (descr_sets.empty()) {
				return 0;
			}
			return static_cast<uint32_t>(descr_sets.size());
		}

		vk::DescriptorSet& get_set(uint32_t set)
		{
			// this is assuming that the sets are correctly ordered - should add some sort of check
			return descr_sets[set];
		}

	private:

		vk::Device device;

		// one for all the sets that will be created
		std::unordered_map<uint32_t, vk::DescriptorSet> descr_sets;	
		
	};

}

