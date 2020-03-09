#pragma once

#include "OEMaths/OEMaths.h"
#include "VulkanAPI/Common.h"
#include "utility/BitSetEnum.h"

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace VulkanAPI
{

// forward declerations
class ImageView;
class VkContext;

/**
 * @brief Flags for subpass dependencies, indicating each subpass barrier property
 */
enum class SubpassFlags : uint64_t
{
    DepthRead,
    StencilRead,
    ColourRead,
    TopOfPipeline,
    BottomOfPipeline,
    Merged,
    MergedBegin,
    MergedEnd,
    Threaded,
    __SENTINEL__
};

class RenderPass
{

public:
    enum class LoadType
    {
        Store,
        Clear,
        DontCare
    };

    enum class StoreType
    {
        Store,
        DontCare
    };

    /**
     * Describes what should be done with the images pre- and post- pass - i.e. keep or throw away
     * the data
     */
    struct ClearFlags
    {
        LoadType attachLoad = LoadType::Clear;
        StoreType attachStore = StoreType::Store;
        LoadType stencilLoad = LoadType::DontCare;
        StoreType stencilStore = StoreType::DontCare;
    };

    RenderPass(VkContext& context);
    ~RenderPass();

    // no copying
    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    // static functions
    static vk::ImageLayout getFinalTransitionLayout(const vk::Format format);
    static vk::ImageLayout getAttachmentLayout(vk::Format format);
    static vk::AttachmentLoadOp loadFlagsToVk(const LoadType flags);
    static vk::AttachmentStoreOp storeFlagsToVk(const StoreType flags);
    static vk::SampleCountFlagBits samplesToVk(const uint32_t count);

    /// Adds a attahment for this pass. This can be a colour or depth attachment
    void addOutputAttachment(
        const vk::Format format,
        const uint32_t reference,
        ClearFlags& clearFlags,
        const uint32_t sampleCount);

    /// adds an input attachment reference. Must have an attachment description added by calling
    /// **addAttachment**
    void addInputRef(const uint32_t reference);

    /// Adds a subpass, the colour outputs and inputs will be linked via the reference ids. These
    /// must have already been added as attachments, otherwise this will throw an error
    bool addSubPass(
        std::vector<uint32_t>& inputRefs,
        std::vector<uint32_t>& outputRefs,
        const uint32_t depthRef = UINT32_MAX);

    void addSubpassDependency(const Util::BitSetEnum<VulkanAPI::SubpassFlags>& flags);

    /// Actually creates the renderpass based on the above definitions
    void prepare();

    // ====================== the getter and setters =================================
    vk::RenderPass& get();

    // kind of replicated from the frame buffer
    uint32_t getWidth() const;
    uint32_t getHeight() const;

    /// sets the clear and depth clear colour - these will only be used if the pass has a colour
    /// and/or depth attachment
    void setClearColour(OEMaths::colour4& col);
    void setDepthClear(float col);

    /// functions that return the state of various aspects of this pass
    bool hasColourAttach();
    bool hasDepthAttach();

    std::vector<vk::PipelineColorBlendAttachmentState> getColourAttachs();

private:
    struct SubpassInfo
    {
        vk::SubpassDescription descr;
        std::vector<vk::AttachmentReference> colourRefs;
        std::vector<vk::AttachmentReference> inputRefs;
        vk::AttachmentReference* depth = nullptr;
    };

    struct OutputReferenceInfo
    {
        vk::AttachmentReference ref;
        size_t index; //< points to the attachment description for this ref.
    };

    friend class CBufferManager;

private:
    /// keep a refernece of the device this pass was created on for destruction purposes
    VkContext& context;

    vk::RenderPass renderpass;

    /// the colour/input attachments
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<OutputReferenceInfo> outputRefs;
    std::vector<vk::AttachmentReference> inputRefs;

    /// subpasses - could be a single or multipass
    std::vector<SubpassInfo> subpasses;

    /// the dependencies between renderpasses and external sources
    std::vector<vk::SubpassDependency> dependencies;

    /// the clear colour for this pass - for each attachment
    OEMaths::colour4 clearCol;
    float depthClear = 0.0f;

    /// max extents of this pass
    uint32_t width = 0;
    uint32_t height = 0;
};

class FrameBuffer
{
public:
   
    FrameBuffer(VkContext& context);
    ~FrameBuffer();

    void prepare(
        RenderPass& rpass,
        std::vector<ImageView*>& imageViews,
        uint32_t width,
        uint32_t height,
        uint32_t layerCount);

    vk::Framebuffer& get()
    {
        return fbuffer;
    }

    uint32_t getWidth() const
    {
        return width;
    }

    uint32_t getHeight() const
    {
        return height;
    }

private:
    // references
    VkContext& context;

    // extents of this buffer
    uint32_t width = 0;
    uint32_t height = 0;

    vk::Framebuffer fbuffer;
};

} // namespace VulkanAPI
