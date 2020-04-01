#include "CBufferManager.h"

#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

CBufferManager::CBufferManager(VkContext& context)
    : context(context)
    , cmdBuffer(std::make_unique<CmdBuffer>(context, cmdPool, CmdBuffer::Type::Primary))
    , workCmdBuffer(std::make_unique<CmdBuffer>(context, cmdPool, CmdBuffer::Type::Primary))
    , scCmdBuffer(std::make_unique<CmdBuffer>(context, cmdPool, CmdBuffer::Type::Primary))
{
    assert(context.device);

    // create the main cmd pool for this buffer - TODO: we should allow for the user to define the
    // queue to use for the pool
    vk::CommandPoolCreateInfo createInfo {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, context.queueFamilyIndex.graphics};
    context.device.createCommandPool(&createInfo, nullptr, &cmdPool);

    createMainDescriptorPool();

    // create the semaphore for signalling a new frame is ready now
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_CHECK_RESULT(context.device.createSemaphore(
        &semaphoreCreateInfo, nullptr, &renderingCompleteSemaphore));
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

DescriptorSetInfo* CBufferManager::findDescriptorSet(uint32_t shaderHash, const uint8_t setValue)
{
    DescriptorSetInfo* descrSet = nullptr;

    DescriptorKey key {shaderHash};
    auto iter = descriptorSets.find(key);

    // if the pipeline has already has an instance return this
    if (iter != descriptorSets.end())
    {
        for (auto& set : iter->second)
        {
            if (set.setValue == setValue)
            {
                descrSet = &set;
            }
        }
    }

    return descrSet;
}

std::vector<DescriptorSetInfo> CBufferManager::findDescriptorSets(uint32_t shaderHash)
{
    std::vector<DescriptorSetInfo> descrSets;

    DescriptorKey key {shaderHash};
    auto iter = descriptorSets.find(key);

    // if the pipeline has already has an instance return this
    if (iter != descriptorSets.end())
    {
        descrSets = iter->second;
        std::sort(
            descrSets.begin(),
            descrSets.end(),
            [](const DescriptorSetInfo& lhs, const DescriptorSetInfo& rhs) {
                return lhs.setValue < rhs.setValue;
            });
    }

    return descrSets;
}

void CBufferManager::addDescriptorLayout(
    uint32_t shaderId,
    const Util::String& layoutId,
    uint32_t set,
    uint32_t bindValue,
    vk::DescriptorType bindType,
    vk::ShaderStageFlags flags)
{
    DescriptorBinding binding;
    binding.layoutId = layoutId;
    binding.set = set;
    binding.binding = vk::DescriptorSetLayoutBinding {bindValue, bindType, 1, flags, nullptr};
    descriptorBindings[shaderId].emplace_back(binding);
}

void CBufferManager::createMainDescriptorPool()
{
    // create a pool for each descriptor type
    std::array<vk::DescriptorPoolSize, 6> pools;

    pools[0] = vk::DescriptorPoolSize {vk::DescriptorType::eUniformBuffer, MaxDescriptorPoolSize};
    pools[1] =
        vk::DescriptorPoolSize {vk::DescriptorType::eCombinedImageSampler, MaxDescriptorPoolSize};
    pools[2] = vk::DescriptorPoolSize {vk::DescriptorType::eStorageBuffer, MaxDescriptorPoolSize};
    pools[3] =
        vk::DescriptorPoolSize {vk::DescriptorType::eUniformBufferDynamic, MaxDescriptorPoolSize};

    pools[4] =
        vk::DescriptorPoolSize {vk::DescriptorType::eStorageBufferDynamic, MaxDescriptorPoolSize};
    pools[5] = vk::DescriptorPoolSize {vk::DescriptorType::eStorageImage, MaxDescriptorPoolSize};

    vk::DescriptorPoolCreateInfo createInfo(
        {}, MaxDescriptorPoolSets, static_cast<uint32_t>(pools.size()), pools.data());
    VK_CHECK_RESULT(context.device.createDescriptorPool(&createInfo, nullptr, &descriptorPool));
}

void CBufferManager::buildDescriptorSets()
{
    for (auto& descrBind : descriptorBindings)
    {
        // sort each layout into its own set
        std::unordered_map<uint8_t, std::vector<vk::DescriptorSetLayoutBinding>> setBindings;

        for (auto& setBind : descrBind.second)
        {
            setBindings[setBind.set].emplace_back(setBind.binding);
        }

        // initialise descriptor pool first based on layouts that have been added
        // create the layout and set
        for (auto& setBind : setBindings)
        {
            DescriptorSetInfo set;
            vk::DescriptorSetLayoutCreateInfo layoutInfo(
                {}, static_cast<uint32_t>(setBind.second.size()), setBind.second.data());
            VK_CHECK_RESULT(
                context.device.createDescriptorSetLayout(&layoutInfo, nullptr, &set.layout));

            // create descriptor set for each layout
            vk::DescriptorSetAllocateInfo allocInfo(descriptorPool, 1, &set.layout);
            VK_CHECK_RESULT(context.device.allocateDescriptorSets(&allocInfo, &set.descrSet));

            DescriptorKey key {descrBind.first};
            descriptorSets[key].emplace_back(set);
        }
    }
}

bool CBufferManager::updateDescriptors(const Util::String& layoutName, Buffer& buffer)
{
    // find the descriptor blueprint first which will then give us the shader id and set value for
    // this buffer
    for (auto& binding : descriptorBindings)
    {
        for (auto& bind : binding.second)
        {
            if (bind.layoutId.compare(layoutName))
            {
                DescriptorSetInfo* setInfo = findDescriptorSet(binding.first, bind.set);
                assert(setInfo);

                vk::DescriptorBufferInfo bufferInfo {
                    buffer.get(), buffer.getOffset(), buffer.getSize()};
                vk::WriteDescriptorSet write {
                    setInfo->descrSet,
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

    LOGGER_ERROR("Unable to find a buffer descriptor with the id %s", layoutName.c_str());
    return false;
}

bool CBufferManager::updateDescriptors(const Util::String& layoutName, Texture& tex)
{
    // find the descriptor blueprint first which will then give us the shader id and set value for
    // this buffer
    for (auto& binding : descriptorBindings)
    {
        for (auto& bind : binding.second)
        {
            if (bind.layoutId.compare(layoutName))
            {
                DescriptorSetInfo* setInfo = findDescriptorSet(binding.first, bind.set);
                assert(setInfo);

                updateDescriptors(
                    bind.binding.binding, bind.binding.descriptorType, setInfo->descrSet, tex);
                return true;
            }
        }
    }

    LOGGER_ERROR("Unable to find a image sampler descriptor with the id %s", layoutName.c_str());
    return false;
}

void CBufferManager::updateDescriptors(
    const uint32_t bindingValue,
    const vk::DescriptorType& type,
    const vk::DescriptorSet& set,
    Texture& tex)
{
    vk::DescriptorImageInfo imageInfo {
        tex.getSampler()->get(), tex.getImageView()->get(), tex.getImageLayout()};
    vk::WriteDescriptorSet write {set, bindingValue, 0, 1, type, &imageInfo, nullptr, nullptr};
    context.device.updateDescriptorSets(1, &write, 0, nullptr);
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

void CBufferManager::flushCmdBuffer()
{
    cmdBuffer->end();

    vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo info {0, nullptr, &flags, 1, &cmdBuffer->get(), 0, nullptr};
    VK_CHECK_RESULT(context.graphicsQueue.submit(1, &info, cmdBuffer->cmdFence));

    // make sure that the cmd buffer has finished before resetting
    VK_CHECK_RESULT(context.device.waitForFences(1, &cmdBuffer->cmdFence, true, UINT64_MAX));
    VK_CHECK_RESULT(context.device.resetFences(1, &cmdBuffer->cmdFence));

    // reset and begin the buffer
    cmdBuffer.get()->reset();
    cmdBuffer->begin();
}

void CBufferManager::flushSwapchainCmdBuffer(
    vk::Semaphore& imageReadySemaphore, Swapchain& swapchain, const uint32_t imageIndex)
{
    scCmdBuffer->end();

    vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo info {
        1, &imageReadySemaphore, &flags, 1, &cmdBuffer->get(), 1, &renderingCompleteSemaphore};
    VK_CHECK_RESULT(context.graphicsQueue.submit(1, &info, scCmdBuffer->cmdFence));

    // and present to the surface backbuffer
    vk::PresentInfoKHR presentInfo {
        1, &renderingCompleteSemaphore, 1, &swapchain.get(), &imageIndex, nullptr};
    VK_CHECK_RESULT(context.presentQueue.presentKHR(&presentInfo));
}

CmdBuffer* CBufferManager::createSecondaryCmdBuffer()
{
    ThreadedCmdBuffer tCmdBuffer;

    // each thread needs it's own cmd pool
    vk::CommandPoolCreateInfo createInfo {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, context.queueFamilyIndex.graphics};
    context.device.createCommandPool(&createInfo, nullptr, &tCmdBuffer.cmdPool);

    tCmdBuffer.secondary =
        std::make_unique<CmdBuffer>(context, tCmdBuffer.cmdPool, CmdBuffer::Type::Secondary);

    // inherit from the main cmd buffer
    tCmdBuffer.secondary->init();
    
    threadedBuffers.emplace_back(std::move(tCmdBuffer));
    return threadedBuffers.back().secondary.get();
}

void CBufferManager::executeSecondaryCommands()
{
    assert(!threadedBuffers.empty());

    // sort all the cmd buffers into a container
    std::vector<vk::CommandBuffer> cmdBuffers;
    for (auto& buffer : threadedBuffers)
    {
        cmdBuffers.emplace_back(buffer.secondary->get());
    }
    workCmdBuffer->get().executeCommands(
        static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
}

CmdBuffer* CBufferManager::getCmdBuffer()
{
    assert(cmdBuffer);
    return cmdBuffer.get();
}

vk::DescriptorPool& CBufferManager::getDescriptorPool()
{
    return descriptorPool;
}

} // namespace VulkanAPI
