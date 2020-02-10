#pragma once

namespace OmegaEngine
{

/**
 * @brief A useful container for grouping together render graph variables for use externally
 */
struct RGraphContext
{
    // the command buffer handle for this pass - not owned here - should maybe be a handle?
    VulkanAPI::CmdBuffer* cmdBuffer = nullptr;

    // the vulkan render pass and framebuffer for this graph context
    RPassHandle rpass;
    FBufferHandle framebuffer;

    // clear colours for this pass
    OEMaths::colour4 clearCol = {0.0f};
    float depthClear = 1.0f;

    // useful vulkan managers - not owned by this struct
    VulkanAPI::VkDriver* driver = nullptr;

    // keep track of the renderer
    OERenderer* renderer = nullptr;

    // the rendergrpah this pass is associated with
    RenderGraph* rGraph = nullptr;
};

using ExecuteFunc = std::function<void(RGraphContext&)>;

class RenderGraphPass
{
public:
    // At the moment, vulkan doesn't support compute subpasses. Thus,
    // if a compute stage is required. Then the renderpass will end, and the
    // compute pass will be deployed, and any remaining graphic passes will be
    // started in another pass after the compute finishes. This isn't ideal performance wise, as
    // switching between different pipelines is expensive, so compute calls should be batched,
    // ideally before the the graphics renderpass
    enum Type
    {
        Graphics,
        Compute
    };

    RenderGraphPass(Util::String name, const Type type, RenderGraph& rGaph);

    // adds a input attachment reader handle to the pass
    ResourceHandle addInput(const ResourceHandle input);

    // adds a colour/depth/stencil attachment writer to the pass
    ResourceHandle addOutput(const ResourceHandle output);

    // A callback function which will be called each frame
    void addExecute(ExecuteFunc&& func);

    // init the vulkan renderpass - attachments, ref, dependencies
    // these can be added to a parent merged pass if defined
    void prepare(VulkanAPI::VkDriver& driver, RenderGraphPass* parent = nullptr);

    // creates the vulkan renderpass. You must call **prepare()** first
    void bake();

    // Sets the clear colour for all attachments for this pass
    void setClearColour(const OEMaths::colour4& clearCol);

    // sets the depth clear for this pass
    void setDepthClear(const float depthClear);

    friend class RenderGraph;

private:
    RenderGraph& rGraph;
    Util::String name;
    Type type;

    // a list of handles of input and output attachments
    std::vector<ResourceHandle> inputs; // input attachments
    std::vector<ResourceHandle> outputs; // colour/depth/stencil attachments

    // the execute function to be used by this pass
    ExecuteFunc execFunc;

    // ====== compiler set =========
    // reference count for the number of outputs
    size_t refCount = 0;

    // the max dimesnions of the resources within this pass.
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;

    // If this pass is mergeable, then this will point to the root pass index
    uint64_t mergedRootIdx = UINT64_MAX;

    // This is only used if this pass will be used threaded, i.e. using secondary cmd buffers. If
    // this is the case, all cmd buffers will be allocated from this pool and will be reset per
    // frame through a call to **update**.
    VulkanAPI::CmdPool* cmdPool = nullptr;

    // flags depicting how the subpasses will behave
    Util::BitSetEnum<VulkanAPI::SubpassFlags> flags;

    // ======= vulkan specific ================
    // Kept in a struct as this will be passed around when rendering passes
    RGraphContext context;
};

} // namespace OmegaEngine