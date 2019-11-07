#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"

#include <tuple>
#include <unordered_map>

namespace VulkanAPI
{

class DescriptorLayout
{

public:
	struct LayoutBindings
	{
		// grouped into sets
		std::unordered_map<uint32_t, std::vector<vk::DescriptorSetLayoutBinding>> layouts;

		// running counts of each descriptor type - required for creating descriptor pools
		// using one pool for multiple sets
		uint32_t uboCount = 0;
		uint32_t ssboCount = 0;
		uint32_t samplerCount = 0;
		uint32_t uboDynamicCount = 0;
		uint32_t ssboDynamicCount = 0;
		uint32_t storageImageCount = 0;
	};

	DescriptorLayout();
	~DescriptorLayout();

	void add(uint32_t set, uint32_t binding, vk::DescriptorType bindType,
	               vk::ShaderStageFlags flags);

	void prepare(vk::Device device, const uint32_t imageSets = 1);

	std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout>> &getLayout()
	{
		assert(!descriptorLayouts.empty());
		return descriptorLayouts;
	}

	vk::DescriptorSetLayout &getLayout(uint32_t set)
	{
		for (auto &layout : descriptorLayouts)
		{
			if (std::get<0>(layout) == set)
			{
				return std::get<1>(layout);
			}
		}
		throw std::runtime_error("Unable to find descriptor layout.");
	}

	vk::DescriptorPool &getDescriptorPool()
	{
		assert(pool);
		return pool;
	}

private:
	vk::Device device;

	vk::DescriptorPool pool;

	// each set will need it's own layout - keep track of the set number so we can match them correctly later
	std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout>> descriptorLayouts;
	LayoutBindings layout_bind;
};

class DescriptorSet
{

public:
	DescriptorSet();
	~DescriptorSet();
	DescriptorSet(vk::Device device, DescriptorLayout descriptorLayout);
	DescriptorSet(vk::Device device, DescriptorLayout descriptorLayout, uint32_t set);

	void init(vk::Device &device, DescriptorLayout &descriptorLayout);
	void init(vk::Device &device, DescriptorLayout &descriptorLayout, uint32_t set);
	void init(vk::Device &device, vk::DescriptorSetLayout &layout, vk::DescriptorPool &pool,
	          uint32_t set);

	void writeSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer &buffer,
	              uint32_t offset, uint32_t range);
	void writeSet(ImageReflection::ShaderImageLayout &imageLayout, vk::ImageView &imageView);

	// use this when you haven't reflected the shader
	void writeSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler &sampler,
	              vk::ImageView &imageView, vk::ImageLayout layout);

	vk::DescriptorSet &get(uint32_t set)
	{
		assert(!descriptorSets.empty());
		return descriptorSets[set];
	}

	std::vector<vk::DescriptorSet> get()
	{
		assert(!descriptorSets.empty());
		// first get the bindings
		std::vector<uint32_t> bindings;
		for (auto &set : descriptorSets)
		{
			bindings.push_back(set.first);
		}

		// sort into ascending order
		std::sort(bindings.begin(), bindings.end());

		// now create the sets
		std::vector<vk::DescriptorSet> sets;
		for (auto &bind : bindings)
		{
			sets.push_back(descriptorSets[bind]);
		}
		return sets;
	}

	uint32_t getSize() const
	{
		if (descriptorSets.empty())
		{
			return 0;
		}
		return static_cast<uint32_t>(descriptorSets.size());
	}

	vk::DescriptorSet &get_set(uint32_t set)
	{
		// this is assuming that the sets are correctly ordered - should add some sort of check
		return descriptorSets[set];
	}

private:
	vk::Device device;

	// one for all the sets that will be created
	std::unordered_map<uint32_t, vk::DescriptorSet> descriptorSets;
};

} // namespace VulkanAPI
