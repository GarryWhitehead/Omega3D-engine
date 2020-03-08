#include "CBufferManager.h"

#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

// ===============================================================================================================


void CmdPool::submitAll(
    Swapchain& swapchain, const uint32_t imageIndex, const vk::Semaphore& beginSemaphore)
{
    vk::Semaphore waitSync;
    vk::Semaphore signalSync;

    for (uint32_t i = 0; i <= cmdInstances.size(); ++i)
    {
        vk::CommandBuffer cmdBuffer;
        vk::Fence fence;

        // work out the signalling and wait semaphores
        if (i == 0)
        {
            waitSync = beginSemaphore;
            signalSync = cmdInstances[i].semaphore;
            cmdBuffer = cmdInstances[i].cmdBuffer->get();
            fence = cmdInstances[i].fence;
        }
        else
        {
            waitSync = cmdInstances[i - 1].semaphore;
            signalSync = cmdInstances[i].semaphore;
            cmdBuffer = cmdInstances[i].cmdBuffer->get();
            fence = cmdInstances[i].fence;
        }

        VK_CHECK_RESULT(context.device.resetFences(1, &fence));

        vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eAllCommands;
        vk::SubmitInfo info {1, &waitSync, &flags, 1, &cmdBuffer, 1, &signalSync};
        VK_CHECK_RESULT(context.graphQueue.submit(1, &info, fence));
    }

    // then the presentation part.....
    vk::Semaphore finalSemaphore = cmdInstances.back().semaphore;
    vk::PresentInfoKHR presentInfo {1, &finalSemaphore, 1, &swapchain.get(), &imageIndex, nullptr};
    VK_CHECK_RESULT(context.presentQueue.presentKHR(&presentInfo));
}

// ================================================================================================================

CBufferManager::CBufferManager(VkContext& context)
    : context(context)
    , cmdBuffer(std::make_unique<CmdBuffer>(context, cmdPool, CmdBuffer::Type::Primary))
    , workCmdBuffer(std::make_unique<CmdBuffer>(context, cmdPool, CmdBuffer::Type::Primary))
    , scCmdBuffer(std::make_unique<CmdBuffer>(context, cmdPool, CmdBuffer::Type::Primary))
{
    assert(context.device);

    // create the main cmd pool for this buffer
    vk::CommandPoolCreateInfo createInfo {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                          queueIndex};
    context.device.createCommandPool(&createInfo, nullptr, &cmdPool);
}

CBufferManager::~CBufferManager()
{
    context.device.destroy(cmdPool, nullptr);
}

Pipeline* CBufferManager::findOrCreatePipeline(ShaderProgram* prog, RenderPass* rPass)
{
    Pipeline* pline = nullptr;

    PLineKey key {prog, rPass};
    auto iter = pipelines.find(key);

    // if the pipeline has already has an instance return this
    if (iter != pipelines.end())
    {
        pline = iter->second;
    }
    else
    {
        // else create a new pipeline - If we are in a threaded environemt then we can't add to the
        // list until we are out of the thread
        pline = new Pipeline(context, *rPass, *prog->getPLineLayout());
        pline->create(*prog);
        pipelines.emplace(key, pline);
    }

    return pline;
}

void CBufferManager::addDescriptorLayout(
    Util::String shaderId,
    Util::String layoutId,
    uint32_t set,
    uint32_t bindValue,
    vk::DescriptorType bindType,
    vk::ShaderStageFlags flags)
{
    DescriptorBinding binding;
    binding.name = layoutId;
    binding.set = set;
    binding.binding = vk::DescriptorSetLayoutBinding {bindValue, bindType, 1, flags, nullptr};
    descriptorBindings[shaderId].emplace_back(binding);
}

void CBufferManager::buildDescriptors(vk::DescriptorPool& pool)
{
    for (auto& binding : descriptorBindings)
    {
        uint32_t uboCount = 0;
        uint32_t ssboCount = 0;
        uint32_t samplerCount = 0;
        uint32_t uboDynamicCount = 0;
        uint32_t ssboDynamicCount = 0;
        uint32_t storageImageCount = 0;

        // sort each layout into its own set
        std::unordered_map<uint8_t, std::vector<vk::DescriptorSetLayoutBinding>> setBindings;
        for (auto& descrBind : binding.second)
        {
            setBindings[descrBind.set].emplace_back(descrBind.binding);

            // Note: not sure the best way here - could create just a few pools and set them to a
            // large
            // enough size to accomodate any eventuality. Though this may lead to issues with
            // needing more pools due to more cmd buffers in flight (descriptors cannot be updated
            // whilst the cmd buffer is queued). Thus, will stick with this method for now calculate
            // the number of each type we have...
            switch (descrBind.binding.descriptorType)
            {
                case vk::DescriptorType::eUniformBuffer:
                    ++uboCount;
                    break;
                case vk::DescriptorType::eStorageBuffer:
                    ++ssboCount;
                    break;
                case vk::DescriptorType::eUniformBufferDynamic:
                    ++uboDynamicCount;
                    break;
                case vk::DescriptorType::eStorageBufferDynamic:
                    ++ssboDynamicCount;
                    break;
                case vk::DescriptorType::eCombinedImageSampler:
                    ++samplerCount;
                    break;
            }
        }

        // initialise descriptor pool first based on layouts that have been added
        std::vector<vk::DescriptorPoolSize> pools;

        if (uboCount)
        {
            vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, uboCount);
            pools.push_back(poolSize);
        }
        if (samplerCount)
        {
            // we can have multiple sets of images - useful in the case of materials for
            // instance
            vk::DescriptorPoolSize poolSize(
                vk::DescriptorType::eCombinedImageSampler, samplerCount);
            pools.push_back(poolSize);
        }
        if (ssboCount)
        {
            vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageBuffer, ssboCount);
            pools.push_back(poolSize);
        }
        if (uboDynamicCount)
        {
            vk::DescriptorPoolSize poolSize(
                vk::DescriptorType::eUniformBufferDynamic, uboDynamicCount);
            pools.push_back(poolSize);
        }
        if (ssboDynamicCount)
        {
            vk::DescriptorPoolSize poolSize(
                vk::DescriptorType::eStorageBufferDynamic, ssboDynamicCount);
            pools.push_back(poolSize);
        }
        if (storageImageCount)
        {
            vk::DescriptorPoolSize poolSize(vk::DescriptorType::eStorageImage, storageImageCount);
            pools.push_back(poolSize);
        }

        // creating a pool requires us to the max sets required for this instance.
        // Occassionally, especially in the case of materials, we may need an extra set which
        // might not be added before a call to here. So we add an extra set.
        uint32_t setCount = static_cast<uint32_t>(setBindings.size()) + 1;

        vk::DescriptorPoolCreateInfo createInfo(
            {}, setCount, static_cast<uint32_t>(pools.size()), pools.data());
        VK_CHECK_RESULT(context.device.createDescriptorPool(&createInfo, nullptr, &pool));

        // create the layouts and sets
        for (auto& setBind : setBindings)
        {
            DescriptorSet set;
            vk::DescriptorSetLayoutCreateInfo layoutInfo(
                {}, static_cast<uint32_t>(setBind.second.size()), setBind.second.data());
            VK_CHECK_RESULT(
                context.device.createDescriptorSetLayout(&layoutInfo, nullptr, &set.layout));

            // create descriptor set for each layout
            vk::DescriptorSetAllocateInfo allocInfo(pool, 1, &set.layout);
            VK_CHECK_RESULT(context.device.allocateDescriptorSets(&allocInfo, &set.set));

            DescriptorKey key {binding.first, setBind.first};
            descriptorSets.emplace(key, set);
        }
    }
}

bool CBufferManager::updateDescriptors(const Util::String& id, Buffer& buffer)
{
    // find the descriptor blueprint first which will then give us the shader id and set value for
    // this buffer
    DescriptorKey key;

    for (auto& binding : descriptorBindings)
    {
        for (auto& bind : binding.second)
        {
            if (bind.name.compare(id))
            {
                // quick sanity check to  make sure this descriptor is a buffer

                key.setValue = bind.set;
                key.id = binding.first;

                DescriptorSet& descrSet = descriptorSets[key];
                vk::DescriptorBufferInfo bufferInfo {
                    buffer.get(), buffer.getOffset(), buffer.getSize()};
                vk::WriteDescriptorSet write {descrSet.set,
                                              bind.binding.binding,
                                              0,
                                              1,
                                              bind.binding.descriptorType,
                                              nullptr,
                                              &bufferInfo,
                                              nullptr};
                context.device.updateDescriptorSets(1, &write, 0, nullptr);
                return true;
            }
        }
    }

    LOGGER_ERROR("Unable to find a buffer descriptor with the id %s", id.c_str());
    return false;
}

bool CBufferManager::updateDescriptors(const Util::String& id, Texture& tex)
{
    // find the descriptor blueprint first which will then give us the shader id and set value for
    // this buffer
    DescriptorKey key;

    for (auto& binding : descriptorBindings)
    {
        for (auto& bind : binding.second)
        {
            if (bind.name.compare(id))
            {
                // quick sanity check to  make sure this descriptor is a image sampler

                key.setValue = bind.set;
                key.id = binding.first;

                DescriptorSet& descrSet = descriptorSets[key];
                vk::DescriptorImageInfo imageInfo {
                    tex.getSampler()->get(), tex.getImageView()->get(), tex.getImageLayout()};
                vk::WriteDescriptorSet write {descrSet.set,
                                              bind.binding.binding,
                                              0,
                                              1,
                                              bind.binding.descriptorType,
                                              &imageInfo,
                                              nullptr,
                                              nullptr};
                context.device.updateDescriptorSets(1, &write, 0, nullptr);
                return true;
            }
        }
    }

    LOGGER_ERROR("Unable to find a image sampler descriptor with the id %s", id.c_str());
    return false;
}

CmdBuffer* CBufferManager::getWorkCmdBuffer()
{
    // make sure that the cmd buffer has finished before resetting
    VK_CHECK_RESULT(context.device.waitForFences(1, &workCmdBuffer->cmdFence, true, UINT64_MAX));
    VK_CHECK_RESULT(context.device.resetFences(1, &workCmdBuffer->cmdFence));

    // reset and begin the buffer
    workCmdBuffer.get()->reset();
    workCmdBuffer->begin();

    return workCmdBuffer.get();
}

CmdBuffer* CBufferManager::getCmdBuffer()
{
    return cmdBuffer.get();
}


} // namespace VulkanAPI
