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

#pragma once

#include "OEMaths/OEMaths.h"
#include "VulkanAPI/Common.h"
#include "utility/BitsetEnum.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class ImageView;
struct VkContext;

/**
 * Describes what should be done with the images pre- and post- pass - i.e. keep or throw away
 * the data. Using a normal enum instaed of the bitsetenum because this will be used in the
 * renderpass key
 */
enum class LoadClearFlags : uint32_t
{
    Store,
    Clear,
    DontCare
};

enum class StoreClearFlags : uint32_t
{
    Store,
    DontCare
};

class RenderPass
{

public:

    enum class DependencyType
    {
        ColourPass,
        DepthPass,
        StencilPass,
        DepthStencilPass
    };

    RenderPass(VkContext& context);
    ~RenderPass();

    // static functions
    static vk::ImageLayout getAttachmentLayout(vk::Format format);
    static vk::AttachmentLoadOp loadFlagsToVk(const LoadClearFlags flags);
    static vk::AttachmentStoreOp storeFlagsToVk(const StoreClearFlags flags);
    static vk::SampleCountFlagBits samplesToVk(const uint32_t count);

    /// Adds a attahment for this pass. This can be a colour or depth attachment
    uint32_t addAttachment(
        const vk::Format format,
        const uint32_t sampleCount,
        const vk::ImageLayout finalLayout,
        LoadClearFlags loadOp,
        StoreClearFlags storeOp,
        LoadClearFlags stencilLoadOp,
        StoreClearFlags stencilStoreOp);

    uint32_t addAttachment(
        const vk::Format format, const uint32_t sampleCount, const vk::ImageLayout finalLayout);

    void addSubpassDependency(DependencyType dependType);

    /// Actually creates the renderpass based on the above definitions and creates the framebuffer
    void prepare();

    // ====================== the getter and setters =================================
    vk::RenderPass& get();

    /// sets the clear and depth clear colour - these will only be used if the pass has a colour
    /// and/or depth attachment
    void setClearColour(OEMaths::colour4& col);
    void setDepthClear(float col);

    /// functions that return the state of various aspects of this pass
    bool hasColourAttach();
    bool hasDepthAttach();
    std::vector<vk::AttachmentDescription>& getAttachments();

    std::vector<vk::PipelineColorBlendAttachmentState> getColourAttachs();

    friend class VkDriver;
    friend class CBufferManager;

private:
    /// keep a refernece of the device this pass was created on for destruction purposes
    VkContext& context;

    vk::RenderPass renderpass;

    /// the colour/input attachments
    std::vector<vk::AttachmentDescription> attachmentDescrs;
    std::vector<vk::AttachmentReference> colourAttachRefs;
    vk::AttachmentReference depthAttachDescr;
    bool hasDepth = false;

    /// subpasses - only one subpass supported at present
    vk::SubpassContents subpass;

    /// the dependencies between renderpasses and external sources
    std::array<vk::SubpassDependency, 2> dependencies;

    /// the clear colour for this pass - for each attachment
    OEMaths::colour4 clearCol {0.0f, 0.0f, 0.0f, 1.0f};
    float depthClear = 1.0f;

    /// max extents of this pass
    uint32_t width = 0;
    uint32_t height = 0;
};

class FrameBuffer
{
public:
    FrameBuffer(VkContext& context);
    ~FrameBuffer();

    void create(
        vk::RenderPass renderpass,
        std::vector<vk::ImageView>& imageViews,
        uint32_t width,
        uint32_t height);

    vk::Framebuffer& get();

    uint32_t getWidth() const;
    uint32_t getHeight() const;

private:
    VkContext& context;

    vk::Framebuffer fbo;

    uint32_t width = 0;
    uint32_t height = 0;
};

} // namespace VulkanAPI
