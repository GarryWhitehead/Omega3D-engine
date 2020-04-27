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

#include "RenderPass.h"

#include "VulkanAPI/Image.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkContext.h"
#include "utility/Logger.h"

#include <assert.h>

namespace VulkanAPI
{

// ============== framebuffer ======================

FrameBuffer::FrameBuffer(VkContext& context) : context(context)
{
}

FrameBuffer::~FrameBuffer()
{
    if (fbo)
    {
        context.device.destroy(fbo);
    }
}

void FrameBuffer::create(vk::RenderPass renderpass, std::vector<vk::ImageView>& imageViews, uint32_t width, uint32_t height)
{
    assert(width > 0);
    assert(height > 0);
    
    this->width = width;
    this->height = height;
    
    // and create the framebuffer.....
    vk::FramebufferCreateInfo fboInfo {
        {},
        renderpass,
        static_cast<uint32_t>(imageViews.size()),
        imageViews.data(),
        width,
        height,
        1};

    VK_CHECK_RESULT(context.device.createFramebuffer(&fboInfo, nullptr, &fbo));
}

vk::Framebuffer& FrameBuffer::get()
{
    return fbo;
}

uint32_t FrameBuffer::getWidth() const
{
    return width;
}

uint32_t FrameBuffer::getHeight() const
{
    return height;
}

// ================== Renderpass ================================

RenderPass::RenderPass(VkContext& context) : context(context)
{
}

RenderPass::~RenderPass()
{
    context.device.destroy(renderpass, nullptr);
}

vk::AttachmentLoadOp RenderPass::loadFlagsToVk(const LoadClearFlags flags)
{
    vk::AttachmentLoadOp result;
    switch (flags)
    {
        case LoadClearFlags::Clear:
            result = vk::AttachmentLoadOp::eClear;
            break;
        case LoadClearFlags::DontCare:
            result = vk::AttachmentLoadOp::eDontCare;
            break;
        case LoadClearFlags::Store:
            result = vk::AttachmentLoadOp::eLoad;
            break;
    }
    return result;
}

vk::AttachmentStoreOp RenderPass::storeFlagsToVk(const StoreClearFlags flags)
{
    vk::AttachmentStoreOp result;
    switch (flags)
    {
        case StoreClearFlags::DontCare:
            result = vk::AttachmentStoreOp::eDontCare;
            break;
        case StoreClearFlags::Store:
            result = vk::AttachmentStoreOp::eStore;
            break;
    }
    return result;
}

vk::SampleCountFlagBits RenderPass::samplesToVk(const uint32_t count)
{
    vk::SampleCountFlagBits result = vk::SampleCountFlagBits::e1;
    switch (count)
    {
        case 1:
            result = vk::SampleCountFlagBits::e1;
            break;
        case 2:
            result = vk::SampleCountFlagBits::e2;
            break;
        case 4:
            result = vk::SampleCountFlagBits::e4;
            break;
        case 8:
            result = vk::SampleCountFlagBits::e8;
            break;
        case 16:
            result = vk::SampleCountFlagBits::e16;
            break;
        case 32:
            result = vk::SampleCountFlagBits::e32;
            break;
        case 64:
            result = vk::SampleCountFlagBits::e64;
            break;
        default:
            LOGGER_WARN("Unsupported sample count. Set to one.");
            break;
    }
    return result;
}

vk::ImageLayout RenderPass::getAttachmentLayout(vk::Format format)
{
    vk::ImageLayout result;
    if (VkUtil::isStencil(format) || VkUtil::isDepth(format))
    {
        result = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
        result = vk::ImageLayout::eColorAttachmentOptimal;
    }
    return result;
}

uint32_t RenderPass::addAttachment(
    const vk::Format format,
    const uint32_t sampleCount,
    const vk::ImageLayout finalLayout,
    LoadClearFlags loadOp,
    StoreClearFlags storeOp,
    LoadClearFlags stencilLoadOp,
    StoreClearFlags stencilStoreOp)
{
    vk::AttachmentDescription attachDescr;
    attachDescr.format = format;
    attachDescr.initialLayout = vk::ImageLayout::eUndefined;
    attachDescr.finalLayout = finalLayout;
    
    // samples
    attachDescr.samples = samplesToVk(sampleCount);

    // clear flags
    attachDescr.loadOp = loadFlagsToVk(loadOp); // pre image state
    attachDescr.storeOp = storeFlagsToVk(storeOp); // post image state
    attachDescr.stencilLoadOp = loadFlagsToVk(stencilLoadOp); // pre stencil state
    attachDescr.stencilStoreOp = storeFlagsToVk(stencilStoreOp); // post stencil state

    attachmentDescrs.emplace_back(attachDescr);

    return attachmentDescrs.size() - 1;
}

uint32_t RenderPass::addAttachment(
const vk::Format format,
const uint32_t sampleCount,
const vk::ImageLayout finalLayout)
{
    return addAttachment(format, sampleCount, finalLayout, LoadClearFlags::Clear, StoreClearFlags::Store, LoadClearFlags::DontCare, StoreClearFlags::DontCare);
}

void RenderPass::addSubpassDependency(DependencyType dependType)
{    
    dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;

    if (dependType == DependencyType::ColourPass)
    {
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].dstAccessMask =
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
    }
    else if (dependType == DependencyType::DepthStencilPass)
    {
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    }
    else if (dependType == DependencyType::StencilPass)
    {
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    }
    else
    {
        LOGGER_INFO(
            "Unsupported dependenciesency read type. This may lead to invalid dst stage masks");
    }
    
    // src and dst stage masks cannot be zero
    assert(dependencies[0].srcStageMask != vk::PipelineStageFlags(0));
    assert(dependencies[0].dstStageMask != vk::PipelineStageFlags(0));

    // and the next dependenciesency stage
    dependencies[1].srcSubpass = dependencies[0].dstSubpass;
    dependencies[1].dstSubpass = dependencies[0].srcSubpass;
    dependencies[1].srcStageMask = dependencies[0].dstStageMask;
    dependencies[1].dstStageMask = dependencies[0].srcStageMask;
    dependencies[1].srcAccessMask = dependencies[0].dstAccessMask;
    dependencies[1].dstAccessMask = dependencies[0].srcAccessMask;
    dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;
}

void RenderPass::prepare()
{
    // create the attachment references
    for (size_t count = 0; count < attachmentDescrs.size(); ++count)
    {
        auto& descr = attachmentDescrs[count];
        
        vk::AttachmentReference ref;
        ref.attachment = count;
        ref.layout = getAttachmentLayout(descr.format);
        
        if (VkUtil::isDepth(descr.format) || VkUtil::isStencil(descr.format))
        {
            hasDepth = true;
            depthAttachDescr = ref;
        }
        else
        {
            colourAttachRefs.emplace_back(ref);
        }
    }
    
    assert(!colourAttachRefs.empty() || hasDepth);
    
     // add the dependdencies
    if (colourAttachRefs.empty())
    {
        // need to check for depth/stencil only here too
        addSubpassDependency(DependencyType::DepthStencilPass);
    }
    else
    {
        addSubpassDependency(DependencyType::ColourPass);
    }
   
    // create the subpass - only one allowed at present until merging is added
    vk::SubpassDescription subpassDescr { {}, vk::PipelineBindPoint::eGraphics, 0, nullptr, static_cast<uint32_t>(colourAttachRefs.size()), colourAttachRefs.data(), nullptr, nullptr, 0, nullptr};
    
    if (hasDepth)
    {
        subpassDescr.pDepthStencilAttachment = &depthAttachDescr;
    }
    
    vk::RenderPassCreateInfo createInfo(
        {},
        static_cast<uint32_t>(attachmentDescrs.size()),
        attachmentDescrs.data(),
        1,
        &subpassDescr,
        static_cast<uint32_t>(dependencies.size()),
        dependencies.data());

    VK_CHECK_RESULT(context.device.createRenderPass(&createInfo, nullptr, &renderpass));
}

vk::RenderPass& RenderPass::get()
{
    assert(renderpass);
    return renderpass;
}

void RenderPass::setClearColour(OEMaths::colour4& col)
{
    clearCol = col;
}

void RenderPass::setDepthClear(float col)
{
    depthClear = col;
}

std::vector<vk::AttachmentDescription>& RenderPass::getAttachments()
{
    return attachmentDescrs;
}

std::vector<vk::PipelineColorBlendAttachmentState> RenderPass::getColourAttachs()
{
    size_t attachCount = attachmentDescrs.size();
    assert(attachCount > 0);
    std::vector<vk::PipelineColorBlendAttachmentState> colAttachs;
    colAttachs.reserve(attachCount);

    // for each clear output colour attachment in the renderpass, we need a blend attachment
    for (uint32_t i = 0; i < attachmentDescrs.size(); ++i)
    {
        // only colour attachments....
        if (VkUtil::isDepth(attachmentDescrs[i].format) || VkUtil::isStencil(attachmentDescrs[i].format))
        {
            continue;
        }
        vk::PipelineColorBlendAttachmentState colour;
        colour.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colour.blendEnable = VK_FALSE; //< TODO: need to add blending
        colAttachs.emplace_back(colour);
    }
    return colAttachs;
}


} // namespace VulkanAPI
