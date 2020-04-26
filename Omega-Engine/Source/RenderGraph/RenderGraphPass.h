#pragma once

#include "RenderGraph/Resources.h"
#include "utility/BitSetEnum.h"
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
struct RGraphContext;

/**
 * @brief A useful container for grouping together data which is specific for this pass - and passed between draw functions
 */
struct RGraphPassContext
{
    // the vulkan render pass
    VulkanAPI::RenderPass* rpass = nullptr;
    VulkanAPI::FrameBuffer* fbo = nullptr;

    // clear colours for this pass
    OEMaths::colour4 clearCol = {0.0f};
    float depthClear = 1.0f;
};

using ExecuteFunc = std::function<void(RGraphPassContext&, RGraphContext&)>;

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
        
    // adds a input attachment reader handle to the pass
    ResourceHandle addRead(const ResourceHandle input);

    // adds a colour/depth/stencil attachment writer to the pass
    ResourceHandle addWrite(const ResourceHandle output);

    // A callback function which will be called each frame
    void addExecute(ExecuteFunc&& func);

    // init the vulkan renderpass - attachments, ref, dependencies
    void prepare(VulkanAPI::VkDriver& driver);

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
    
    // a list of handles of input and output attachments
    std::vector<ResourceHandle> reads; // input attachments
    std::vector<ResourceHandle> writes; // colour/depth/stencil attachments

    // the execute function to be used by this pass
    ExecuteFunc execFunc;

    // ====== compiler set =========
    // the max dimesnions of the resources within this pass.
    uint32_t maxWidth = 0;
    uint32_t maxHeight = 0;
    
    // ======= vulkan specific ================
    // Kept in a struct as this will be passed around when rendering passes
    RGraphPassContext context;
};

} // namespace OmegaEngine
