#pragma once

#include "OEMaths/OEMaths.h"
#include "RenderGraph/Resources.h"
#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/RenderPass.h"
#include "utility/BitSetEnum.h"
#include "utility/CString.h"

#include <functional>
#include <vector>

// forward decleartion
namespace VulkanAPI
{
class ProgramManager;
class CmdBufferManager;
class CmdBuffer;
class FrameBuffer;
class RenderPass;
class VkDriver;
} // namespace VulkanAPI

namespace OmegaEngine
{

// forward declerations
class RenderGraph;
class OERenderer;
class RenderGraphPass;

class VkHandle
{
public:
    VkHandle() = delete;
    explicit VkHandle(const uint64_t handle) : handle(handle)
    {
    }

    uint64_t get()
    {
        return handle;
    }

private:
    uint64_t handle;
};

using FBufferHandle = typename VkHandle;
using RPassHandle = typename VkHandle;

class RenderGraph
{
public:
    RenderGraph(VulkanAPI::VkDriver& driver);
    ~RenderGraph();

    // not copyable or moveable
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = delete;
    RenderGraph& operator=(RenderGraph&&) = delete;

    /**
     * @brief Creates a new renderpass.
     * @param name The name of this pass
     * @ return A convience render graph builder which is used by the user to create the defined
     * pass
     */
    RenderGraphBuilder createPass(Util::String name, const RenderGraphPass::Type type);

    /**
     * @brief Takes the user-defined graph and builds the render pass.
     * The following procedures are carried out:
     * 1. optimisation of the graph and compilation
     * 2. init render passes, framebuffers and command buffers for each pass
     * 3.
     */
    bool prepare();

    /**
     * The execution of the render pass. You must build the pass and call **prepare** before this
     * function
     */
    void execute();

    RPassHandle createRenderPass();
    FBufferHandle createFrameBuffer();

    // ============== getters ==================
    std::vector<ResourceBase*>& getResources();
    ResourceBase* getResource(const ResourceHandle handle);

    VulkanAPI::RenderPass* getRenderpass(RPassHandle& handle);
    VulkanAPI::FrameBuffer* getFramebuffer(FBufferHandle& handle);

    friend class RenderGraphPass;
    friend class RenderGraphBuilder;

private:
    void CullResourcesAndPasses(ResourceBase* resource);
    ResourceHandle addResource(ResourceBase* resource);
    AttachmentHandle addAttachment(AttachmentInfo& info);
    AttachmentHandle findAttachment(const Util::String& attach);

    void initRenderPass();

    // optimises the render graph if possible and fills in all the blanks - i.e. referneces, flags,
    // etc.
    bool compile();

private:
    VulkanAPI::VkDriver& driver;

    // a list of all the render passes
    std::vector<RenderGraphPass> rGraphPasses;

    // a virtual list of all the resources associated with this graph
    std::vector<ResourceBase*> resources;

    // The entirety of the attachments for this graph
    std::vector<AttachmentInfo> attachments;

    // all allocated renderpasses and framebuffers are registered here.
    std::vector<std::unique_ptr<VulkanAPI::RenderPass>> renderpasses;
    std::vector<std::unique_ptr<VulkanAPI::FrameBuffer>> framebuffers;

    bool rebuild = true;
};

} // namespace OmegaEngine
