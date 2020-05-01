/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "CBufferManager.h"

#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/SwapChain.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkContext.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace VulkanAPI
{

CBufferManager::CBufferManager(VkDriver& driver)
    : driver(driver)
    , cmdBuffer(std::make_unique<CmdBuffer>(driver.getContext(), cmdPool, CmdBuffer::Type::Primary))
    , workCmdBuffer(
          std::make_unique<CmdBuffer>(driver.getContext(), cmdPool, CmdBuffer::Type::Primary))
{
    // create the main cmd pool for this buffer - TODO: we should allow for the user to define the
    // queue to use for the pool
    VkContext& context = driver.getContext();
    vk::CommandPoolCreateInfo createInfo {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                          context.queueFamilyIndex.graphics};
    context.device.createCommandPool(&createInfo, nullptr, &cmdPool);

    createMainDescriptorPool();

    // create the semaphore for signalling a new frame is ready now
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    VK_CHECK_RESULT(
        context.device.createSemaphore(&semaphoreCreateInfo, nullptr, &renderingCompleteSemaphore));

    // initialise the cmd buffers
    workCmdBuffer->init();
    cmdBuffer->init();

    // create the cmd buffers for the swap chain backbuffers - we go for the max possible amount
    // which should be if triple buffering, three image views so three cmd buffers
    for (uint32_t i = 0; i < MaxSwapChainCmdBufferSize; ++i)
    {
        scCmdBuffer[i] =
            std::make_unique<CmdBuffer>(driver.getContext(), cmdPool, CmdBuffer::Type::Primary);
        scCmdBuffer[i]->init();
    }

    // create the secondary cmd buffers
    // TODO: make this optional and the number of threaded buffers user defined
    createSecondaryCmdBuffers();
}

CBufferManager::~CBufferManager()
{
    driver.getContext().device.destroy(cmdPool, nullptr);
    driver.getContext().device.destroy(descriptorPool, nullptr);
}

Pipeline* CBufferManager::findOrCreatePipeline(
    ShaderProgram* prog, RenderPass* rpass, FrameBuffer* fbo, Pipeline::Type type)
{
    assert(rpass);
    assert(fbo);

    Pipeline* pline = nullptr;

    PLineKey key {prog, rpass};
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
        pline = new Pipeline(driver.getContext(), *prog->getPLineLayout(), type);
        pline->create(*prog, rpass, fbo);
        pipelines.emplace(key, pline);
    }

    return pline;
}

bool CBufferManager::hasDescriptorSet(uint32_t shaderHash)
{
    DescriptorKey key {shaderHash};
    auto iter = descriptorSets.find(key);
    if (iter == descriptorSets.end())
    {
        return false;
    }
    return true;
}

DescriptorSetInfo* CBufferManager::findDescriptorSet(uint32_t shaderHash, const uint8_t setValue)
{
    if (descriptorSets.empty())
    {
        LOGGER_ERROR("You are trying to find a descriptor set when none have yet been registered.");
        return nullptr;
    }

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
            else
            {
                LOGGER_ERROR(
                    "Descriptor with id: %i has no set %i registered", shaderHash, setValue);
            }
        }
    }
    else
    {
        LOGGER_ERROR("Unable to find descriptor set with id: %i", shaderHash);
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
    printf(
        "Registered descriptor layouut for shader id: %ul; layout name: %s; set: %i; binding: %i\n",
        shaderId,
        binding.layoutId.c_str(),
        set,
        bindValue);
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
    VK_CHECK_RESULT(
        driver.getContext().device.createDescriptorPool(&createInfo, nullptr, &descriptorPool));
}

void CBufferManager::buildDescriptorSet(uint32_t shaderId, PipelineLayout* plineLayout)
{
    assert(shaderId > 0);
    auto& bindings = descriptorBindings[shaderId];

    // check that we have bindings first though as shaders may not have any descriptors associated
    // with them
    if (bindings.empty())
    {
        return;
    }

    // sort each layout into its own set - we store the layout binding with the name of the Ubo
    std::unordered_map<uint8_t, std::vector<vk::DescriptorSetLayoutBinding>> setBindings;

    for (auto& setBind : bindings)
    {
        if (hasDescriptorSet(shaderId))
        {
            continue;
        }
        setBindings[setBind.set].emplace_back(setBind.binding);
    }

    // TODO: check if the descriptor set already exsists within the map so not init more than once.
    vk::Device& device = driver.getContext().device;
    for (auto& setBind : setBindings)
    {
        DescriptorSetInfo set;
        set.setValue = setBind.first;

        vk::DescriptorSetLayoutCreateInfo layoutInfo(
            {}, static_cast<uint32_t>(setBind.second.size()), setBind.second.data());
        VK_CHECK_RESULT(device.createDescriptorSetLayout(&layoutInfo, nullptr, &set.layout));

        // create descriptor set for each layout
        vk::DescriptorSetAllocateInfo allocInfo(descriptorPool, 1, &set.layout);
        VK_CHECK_RESULT(device.allocateDescriptorSets(&allocInfo, &set.descrSet));

        DescriptorKey key {shaderId};
        descriptorSets[key].emplace_back(set);

        // also update the appropiate pipeline layout with this descriptor layout
        plineLayout->addDescriptorLayout(set.setValue, set.layout);
    }
}

bool CBufferManager::updateAllShaderDecsriptorSets()
{
    for (auto& descrBinding : descriptorBindings)
    {
        if (!updateShaderDescriptorSets(descrBinding.first))
        {
            return false;
        }
    }
    return true;
}

bool CBufferManager::updateShaderDescriptorSets(uint32_t shaderId)
{
    auto iter = descriptorBindings.find(shaderId);
    if (iter == descriptorBindings.end())
    {
        LOGGER_ERROR("Unable to find descriptor binding for shader Id %i.", shaderId);
        return false;
    }

    // sort each layout into its own set - we store the layout binding with the name of the Ubo
    std::
        unordered_map<uint8_t, std::vector<std::pair<Util::String, vk::DescriptorSetLayoutBinding>>>
            setBindings;

    for (auto setBind : iter->second)
    {
        setBindings[setBind.set].emplace_back(std::make_pair(setBind.layoutId, setBind.binding));
    }

    // now update the descriptor set with the image/buffer data - important that the shaders and
    // ubos have been created at this point otherwise this will fail
    for (auto& setBind : setBindings)
    {
        DescriptorSetInfo* setInfo = findDescriptorSet(shaderId, setBind.first);
        assert(setInfo);

        for (auto& layoutBinds : setBind.second)
        {
            if (!setInfo->needsUpdate)
            {
                continue;
            }

            vk::DescriptorSetLayoutBinding& bind = layoutBinds.second;
            updateDescriptors(
                bind.binding, bind.descriptorType, setInfo->descrSet, layoutBinds.first);
        }
        setInfo->needsUpdate = false;
    }

    return true;
}

void CBufferManager::updateDescriptors(
    const uint32_t bindingValue,
    const vk::DescriptorType& type,
    const vk::DescriptorSet& set,
    const Util::String& layoutId)
{
    assert(set);

    if (VkUtil::isBufferType(type))
    {
        Buffer* buffer = driver.getBuffer(layoutId);
        assert(buffer);

        vk::DescriptorBufferInfo bufferInfo {
            buffer->get(), 0 /* buffer->getOffset() */, buffer->getSize()};
        vk::WriteDescriptorSet write {set, bindingValue, 0, 1, type, nullptr, &bufferInfo, nullptr};
        driver.getContext().device.updateDescriptorSets(1, &write, 0, nullptr);
    }
    else if (VkUtil::isSamplerType(type))
    {
        Texture* tex = driver.getTexture2D(layoutId);
        assert(tex);

        vk::DescriptorImageInfo imageInfo {
            tex->getSampler(), tex->getImageView()->get(), tex->getImageLayout()};
        vk::WriteDescriptorSet write {set, bindingValue, 0, 1, type, &imageInfo, nullptr, nullptr};
        driver.getContext().device.updateDescriptorSets(1, &write, 0, nullptr);
    }
    else
    {
        printf("Unsupported descriptor type found whilst trying to update descriptor set");
    }
}

void CBufferManager::updateTextureDescriptor(
    const uint32_t bindingValue,
    const vk::DescriptorType& type,
    const vk::DescriptorSet& set,
    Texture* tex)
{
    assert(set);
    vk::DescriptorImageInfo imageInfo {
        tex->getSampler(), tex->getImageView()->get(), tex->getImageLayout()};
    vk::WriteDescriptorSet write {set, bindingValue, 0, 1, type, &imageInfo, nullptr, nullptr};
    driver.getContext().device.updateDescriptorSets(1, &write, 0, nullptr);
}

CmdBuffer* CBufferManager::getScCommandBuffer(uint8_t idx)
{
    assert(idx < scCmdBuffer.size());

    CmdBuffer* cmdBuffer = scCmdBuffer[idx].get();
    cmdBuffer->begin();
    
    return cmdBuffer;
}

CmdBuffer* CBufferManager::getWorkCmdBuffer()
{
    // make sure that the cmd buffer has finished before resetting
    vk::Device& device = driver.getContext().device;

    if (workCmdBuffer->workSubmitted)
    {
        VK_CHECK_RESULT(device.waitForFences(1, &workCmdBuffer->cmdFence, false, UINT64_MAX));
        VK_CHECK_RESULT(device.resetFences(1, &workCmdBuffer->cmdFence));
        workCmdBuffer->workSubmitted = false;

        // reset and begin the buffer
        workCmdBuffer.get()->resetCmdBuffer();
    }
    workCmdBuffer->begin();

    return workCmdBuffer.get();
}

void CBufferManager::flushCmdBuffer()
{
    VkContext& context = driver.getContext();

    cmdBuffer->end();

    vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo info {0, nullptr, &flags, 1, &cmdBuffer->get(), 0, nullptr};
    VK_CHECK_RESULT(context.graphicsQueue.submit(1, &info, cmdBuffer->cmdFence));

    // make sure that the cmd buffer has finished before resetting
    VK_CHECK_RESULT(context.device.waitForFences(1, &cmdBuffer->cmdFence, false, UINT64_MAX));
    VK_CHECK_RESULT(context.device.resetFences(1, &cmdBuffer->cmdFence));

    // reset the buffer
    cmdBuffer.get()->resetCmdBuffer();
}

void CBufferManager::flushSwapchainCmdBuffer(
    vk::Semaphore& imageReadySemaphore, Swapchain& swapchain, const uint32_t imageIndex)
{
    VkContext& context = driver.getContext();

    CmdBuffer* cmdBuffer = scCmdBuffer[imageIndex].get();
    cmdBuffer->end();

    vk::PipelineStageFlags flags = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo info {
        1, &imageReadySemaphore, &flags, 1, &cmdBuffer->get(), 1, &renderingCompleteSemaphore};
    VK_CHECK_RESULT(context.graphicsQueue.submit(1, &info, cmdBuffer->cmdFence));

    // and present to the surface backbuffer
    vk::PresentInfoKHR presentInfo {
        1, &renderingCompleteSemaphore, 1, &swapchain.get(), &imageIndex, nullptr};
    VK_CHECK_RESULT(context.presentQueue.presentKHR(&presentInfo));

    VK_CHECK_RESULT(context.device.waitForFences(1, &cmdBuffer->cmdFence, false, UINT64_MAX));
    VK_CHECK_RESULT(context.device.resetFences(1, &cmdBuffer->cmdFence));

    // reset and begin the buffer
    cmdBuffer->resetCmdBuffer();
}

void CBufferManager::createSecondaryCmdBuffers()
{
    VkContext& context = driver.getContext();

    for (uint32_t i = 0; i < MaxSecondaryCmdBufferSize; ++i)
    {
        ThreadedCmdBuffer tCmdBuffer;

        // each thread needs it's own cmd pool
        vk::CommandPoolCreateInfo createInfo {vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                              context.queueFamilyIndex.graphics};
        context.device.createCommandPool(&createInfo, nullptr, &tCmdBuffer.cmdPool);

        tCmdBuffer.secondary =
            std::make_unique<CmdBuffer>(context, tCmdBuffer.cmdPool, CmdBuffer::Type::Secondary);

        // inherit from the main cmd buffer
        tCmdBuffer.secondary->init();

        threadedBuffers[i] = std::move(tCmdBuffer);
    }
}

CmdBuffer* CBufferManager::getSecondaryCmdBuffer()
{
    if (nextSecondaryCmdBufferIdx >= MaxSecondaryCmdBufferSize)
    {
        LOGGER_WARN("Unable to retrive secondary cmd buffer - max count reached");
        return nullptr;
    }

    return threadedBuffers[nextSecondaryCmdBufferIdx++].secondary.get();
}

void CBufferManager::executeSecondaryCommands()
{
    assert(!threadedBuffers.empty());

    // sort all the cmd buffers into a container
    std::vector<vk::CommandBuffer> cmdBuffers;
    for (uint32_t i = 0; i < nextSecondaryCmdBufferIdx; ++i)
    {
        cmdBuffers.emplace_back(threadedBuffers[i].secondary->get());
        threadedBuffers[i].isExecuted = true;
    }
    cmdBuffer->get().executeCommands(static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
}

void CBufferManager::resetSecondaryCommands()
{
    VkContext& context = driver.getContext();

    nextSecondaryCmdBufferIdx = 0;

    for (auto& secondary : threadedBuffers)
    {
        if (secondary.isExecuted)
        {
            secondary.secondary =
                std::make_unique<CmdBuffer>(context, secondary.cmdPool, CmdBuffer::Type::Secondary);
            secondary.secondary->init();
            secondary.isExecuted = false;
        }
    }
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
