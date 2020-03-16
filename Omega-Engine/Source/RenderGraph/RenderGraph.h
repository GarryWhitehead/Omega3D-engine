#pragma once

#include "OEMaths/OEMaths.h"
#include "RenderGraph/RenderGraphPass.h"
#include "RenderGraph/RenderHandle.h"
#include "RenderGraph/Resources.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/RenderPass.h"
#include "utility/BitSetEnum.h"
#include "utility/CString.h"

#include <functional>
#include <memory>
#include <vector>

// forward decleartion
namespace VulkanAPI
{
class ProgramManager;
class CBufferManager;
class CmdBuffer;
class FrameBuffer;
class RenderPass;
class VkDriver;
class ImageView;
} // namespace VulkanAPI

namespace OmegaEngine
{

// forward declerations
class RenderGraph;
class OERenderer;
class RenderGraphPass;
class RenderGraphBuilder;

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

    ResourceHandle importResource(
        const Util::String& name,
        VulkanAPI::ImageView& imageView,
        const uint32_t width,
        const uint32_t height);

    ResourceHandle moveResource(const ResourceHandle from, const ResourceHandle to);

    std::vector<ResourceBase*>& getResources();
    ResourceBase* getResource(const ResourceHandle handle);

    VulkanAPI::RenderPass* getRenderpass(const RPassHandle& handle);
    VulkanAPI::FrameBuffer* getFramebuffer(const FBufferHandle& handle);

    friend class RenderGraphPass;
    friend class RenderGraphBuilder;

private:
    void CullResourcesAndPasses(ResourceBase* resource);
    ResourceHandle addResource(ResourceBase* resource);
    AttachmentHandle addAttachment(AttachmentInfo& info);
    AttachmentHandle findAttachment(const Util::String& attach);

    void initRenderPass();

    // optimises the render graph if possible and fills in all the blanks - i.e. references, flags,
    // etc.
    bool compile();

private:
    VulkanAPI::VkDriver& driver;

    // a list of all the render passes
    std::vector<RenderGraphPass> rGraphPasses;

    // a virtual list of all the resources associated with this graph
    std::vector<ResourceBase*> resources;

    // used for moving resources - pair = from, to handles
    std::vector<std::pair<ResourceHandle, ResourceHandle>> aliases;

    // The entirety of the attachments for this graph
    std::vector<AttachmentInfo> attachments;

    // all allocated renderpasses and framebuffers are registered here.
    std::vector<std::unique_ptr<VulkanAPI::RenderPass>> renderpasses;
    std::vector<std::unique_ptr<VulkanAPI::FrameBuffer>> framebuffers;

    bool rebuild = true;
};

} // namespace OmegaEngine
