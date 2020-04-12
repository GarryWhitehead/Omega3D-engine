#pragma once

#include "RenderGraph/RenderHandle.h"
#include "RenderGraph/Resources.h"
#include "utility/BitsetEnum.h"
#include "utility/CString.h"

#include <functional>

namespace VulkanAPI
{
class VkDriver;
class CmdBuffer;
class CmdPool;
} // namespace VulkanAPI

namespace OmegaEngine
{

class RenderGraph;
class OERenderer;

/**
 * @brief A useful container for grouping together render graph variables for use externally
 */
struct RGraphContext
{
    // the vulkan render pass and framebuffer for this graph context
    RPassHandle rpass;
    FBufferHandle framebuffer;

    // clear colours for this pass
    OEMaths::colour4 clearCol = {0.0f};
    float depthClear = 1.0f;

    // useful vulkan managers - not owned by this struct
    VulkanAPI::VkDriver* driver = nullptr;
    //OERenderer* renderer = nullptr;
    RenderGraph* rGraph = nullptr;
};

using ExecuteFunc = std::function<void(RGraphContext&)>;

// flags for creating render passes
enum class RenderPassFlags : uint64_t
{
    // If set, this pass will only be executed once until the flag is reset
    IntermitentPass,
    None,
    __SENTINEL__
};

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

    RenderGraphPass(Util::String name, const Type type, RenderGraph& rGaph, const uint32_t index);
    
    void setFlag(const RenderPassFlags& flag);
    
    // adds a input attachment reader handle to the pass
    ResourceHandle addRead(const ResourceHandle input);

    // adds a colour/depth/stencil attachment writer to the pass
    ResourceHandle addWrite(const ResourceHandle output);

    // A callback function which will be called each frame
    void addExecute(ExecuteFunc&& func);

    // init the vulkan renderpass - attachments, ref, dependencies
    void prepare(VulkanAPI::VkDriver& driver);

    // creates the vulkan renderpass. You must call **prepare()** first
    void bake();

    // Sets the clear colour for all attachments for this pass
    void setClearColour(const OEMaths::colour4& clearCol);

    // sets the depth clear for this pass
    void setDepthClear(const float depthClear);
    
    // resets the skip execute flag, so the pass will be executed on the next frame
    void resetSkipExecFlag();

    friend class RenderGraph;

private:
    RenderGraph& rGraph;
    Util::String name;
    Type type;

    const uint32_t index = 0;
    
    // flags which alter the behaviour of the pass
    Util::BitSetEnum<RenderPassFlags> renderPassFlags;
    
    // used by the interminent render flag, this states whether the pass should be executed
    bool skipPassExec = false;
    
    // a list of handles of input and output attachments
    std::vector<ResourceHandle> reads; // input attachments
    std::vector<ResourceHandle> writes; // colour/depth/stencil attachments

    // the execute function to be used by this pass
    ExecuteFunc execFunc;

    // ====== compiler set =========
    // the max dimesnions of the resources within this pass.
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;

    // If this pass is mergeable, then this will point to the root pass index
    uint64_t mergedRootIdx = UINT64_MAX;

    // flags depicting how the subpasses will behave
    Util::BitSetEnum<VulkanAPI::SubpassFlags> flags;
    
    // ======= vulkan specific ================
    // Kept in a struct as this will be passed around when rendering passes
    RGraphContext context;
};

} // namespace OmegaEngine
