#pragma once
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Shader.h"

#include <cassert>
#include <tuple>
#include <unordered_map>

namespace VulkanAPI
{

// forward declerations
class VkContext;
class DescriptorLayout;

class DescriptorPool
{
public:
    DescriptorPool(VkContext& context);
    ~DescriptorPool();

    /**
     * @brief running counts of each descriptor type - required for creating descriptor pools using
     * one pool for multiple sets
     */
    struct BindingPool
    {
        /// grouped into sets
        std::unordered_map<uint8_t, std::vector<vk::DescriptorSetLayoutBinding>> layouts;

        uint32_t uboCount = 0;
        uint32_t ssboCount = 0;
        uint32_t samplerCount = 0;
        uint32_t uboDynamicCount = 0;
        uint32_t ssboDynamicCount = 0;
        uint32_t storageImageCount = 0;
    };

    DescriptorLayout& createLayout(
        uint32_t set, uint32_t binding, vk::DescriptorType bindType, vk::ShaderStageFlags flags);

    /**
     * @brief This should be called once all layouts have been added to the pool. This will create
     * the descriptor pool with the appropiate number of descriptor bindings. If further layouts
     * need to be added after building, you must call **rebuild**.
     */
    void build();

    /**
     * @brief Creates all the layouts associated with this pool. Note: If you wish to only create a
     * sub section of layouts, then you can call the descriptor layout function **prepare**
     * directly.
     */
    void prepareLayouts();

    std::vector<vk::DescriptorSetLayout> getLayouts();

    vk::DescriptorPool& get()
    {
        assert(pool);
        return pool;
    }

    DescriptorLayout& findSet(uint8_t setNum);

    bool isEmpty() const
    {
        return layouts.empty();
    }

private:
    VkContext& context;

    /// a running tally of all the different resources associated with this layout
    BindingPool bindings;

    /// Each layout has its own pool - this is to avoid issues if used in a multi-threaded
    /// environment as the spec states: "that the application must not allocate and/or free
    /// descriptor sets from the same pool in multiple threads simultaneously."
    vk::DescriptorPool pool;

    /// a list of all the descriptor layouts associated with this pool
    std::vector<DescriptorLayout> layouts;
};

/**
 * A simple wrapper which contains the layout information for creating a set
 */
class DescriptorLayout
{

public:
    DescriptorLayout(VkContext& context, DescriptorPool& pool);
    ~DescriptorLayout();

    void prepare();

    vk::DescriptorSetLayout& get()
    {
        assert(descrlayout);
        return descrlayout;
    }

    friend class DescriptorPool;
    friend class DescriptorSet;

private:
    VkContext& context;

    /// the pool associated with this layout
    DescriptorPool& pool;

    /// the binding set number for this layout
    uint8_t set = 0;

    // layouts required for building the set
    vk::DescriptorSetLayoutBinding layoutBinding;
    vk::DescriptorSetLayout descrlayout;
};

class DescriptorSet
{

public:
    DescriptorSet(VkContext& context);
    ~DescriptorSet();

    /**
     * @brief Adds a descriptor layout to this set
     */
    void addLayout(DescriptorLayout& descriptorLayout);

    void updateBufferSet(
        uint32_t set,
        uint32_t binding,
        vk::DescriptorType type,
        const vk::Buffer& buffer,
        uint32_t offset,
        uint32_t range);

    void updateImageSet(
        uint32_t set,
        uint32_t binding,
        vk::DescriptorType type,
        vk::Sampler& sampler,
        vk::ImageView& imageView,
        vk::ImageLayout layout);


    vk::DescriptorSet& get(uint32_t set)
    {
        assert(!descrSets.empty());
        return descrSets[set];
    }

    std::vector<vk::DescriptorSet> getOrdered()
    {
        assert(!descrSets.empty());
        // first get the bindings
        std::vector<uint32_t> bindings;
        for (auto& set : descrSets)
        {
            bindings.push_back(set.first);
        }

        // sort into ascending order
        std::sort(bindings.begin(), bindings.end());

        // now create the sets
        std::vector<vk::DescriptorSet> sets;
        for (auto& bind : bindings)
        {
            sets.push_back(descrSets[bind]);
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

    /// one for all the sets that will be created
    std::unordered_map<uint8_t, vk::DescriptorSet> descrSets;
};

} // namespace VulkanAPI
