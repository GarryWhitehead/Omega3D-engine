#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"

#include <cassert>
#include <tuple>
#include <unordered_map>

namespace VulkanAPI
{

/**
 * A descriptor layout describing the bindings for a entire shader program so may have multiple sets
 * *Maintains its own descriptor pool so this can be used in a threaded environment.
 */
class DescriptorLayout
{

public:
	/**
     * @brief running counts of each descriptor type - required for creating descriptor pools using one pool for multiple sets
     */
	struct BindingPool
	{
		// grouped into sets
		std::unordered_map<uint8_t, std::vector<vk::DescriptorSetLayoutBinding>> layouts;

		uint32_t uboCount = 0;
		uint32_t ssboCount = 0;
		uint32_t samplerCount = 0;
		uint32_t uboDynamicCount = 0;
		uint32_t ssboDynamicCount = 0;
		uint32_t storageImageCount = 0;
	};

	DescriptorLayout(VkContext& context);
	~DescriptorLayout();

	void add(uint32_t set, uint32_t binding, vk::DescriptorType bindType, vk::ShaderStageFlags flags);

	void prepare();

	std::vector<std::tuple<uint32_t, vk::DescriptorSetLayout>>& getLayout()
	{
		assert(!descriptorLayouts.empty());
		return descriptorLayouts;
	}

	vk::DescriptorSetLayout& getLayout(uint8_t set)
	{
		auto iter = layouts.find(set);
		assert(iter != layouts.end);
		return iter->second;
	}

	vk::DescriptorPool& getDescriptorPool()
	{
		assert(pool);
		return pool;
	}

	friend class DescriptorSet;

private:
	VkContext& context;

	// Each layout has its own pool - this is to avoid issues if used in a multi-threaded environment as the spec states:
	// "that the application must not allocate and/or free descriptor sets from the same pool in multiple threads simultaneously."
	vk::DescriptorPool pool;

	// each set will need it's own layout - keep track of the set number so we can match them correctly later
	std::unordered_map<uint8_t, vk::DescriptorSetLayout> layouts;

	// a running tally of all the different resources associated with this layout
	BindingPool bindings;
};

class DescriptorSet
{

public:
	DescriptorSet(VkContext& context);
	~DescriptorSet();

	// not copyable
	DescriptorSet(const DescriptorSet&) = delete;
	DescriptorSet operator=(const DescriptorSet&) = delete;

	/**
     * 
     */
	void prepare(DescriptorLayout& descriptorLayout);

	void updateBufferSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Buffer& buffer, uint32_t offset,
	                     uint32_t range);

	void updateImageSet(uint32_t set, uint32_t binding, vk::DescriptorType type, vk::Sampler& sampler,
	                    vk::ImageView& imageView, vk::ImageLayout layout);


	vk::DescriptorSet& get(uint32_t set)
	{
		assert(!descriptorSets.empty());
		return descrSets[set];
	}

	std::vector<vk::DescriptorSet> getOrdered()
	{
		assert(!descriptorSets.empty());
		// first get the bindings
		std::vector<uint32_t> bindings;
		for (auto& set : descriptorSets)
		{
			bindings.push_back(set.first);
		}

		// sort into ascending order
		std::sort(bindings.begin(), bindings.end());

		// now create the sets
		std::vector<vk::DescriptorSet> sets;
		for (auto& bind : bindings)
		{
			sets.push_back(descriptorSets[bind]);
		}
		return sets;
	}

	size_t getSize() const
	{
		return descrSets.size();
	}

	friend class DescriptorLayout;

private:
	VkContext& context;

	// one for all the sets that will be created
	std::unordered_map<uint8_t, vk::DescriptorSet> descrSets;
};

}    // namespace VulkanAPI
