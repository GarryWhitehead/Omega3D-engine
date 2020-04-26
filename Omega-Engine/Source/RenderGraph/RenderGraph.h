#pragma once

#include "OEMaths/OEMaths.h"
#include "RenderGraph/RenderGraphPass.h"
#include "RenderGraph/Resources.h"
#include "VulkanAPI/CBufferManager.h"
#include "VulkanAPI/RenderPass.h"
#include "utility/BitsetEnum.h"
#include "utility/CString.h"

#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

// forward decleartion
namespace VulkanAPI
{
class ProgramManager;
class CBufferManager;
class CmdBuffer;
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

struct RGraphContext
{
    RGraphContext() = default;
    RGraphContext(VulkanAPI::VkDriver* driver, OERenderer* renderer, RenderGraph* rGraph) :
        driver(driver), renderer(renderer), rGraph(rGraph)
    {}
    
    VulkanAPI::VkDriver* driver = nullptr;
    OERenderer* renderer = nullptr;
    RenderGraph* rGraph = nullptr;
};

class RenderGraph
{
public:
    
    RenderGraph(VulkanAPI::VkDriver* driver, OERenderer* renderer);
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

    // optimises the render graph if possible and fills in all the blanks - i.e. references, flags,
       // etc.
       bool compile();
    
    /**
     * The execution of the render pass. You must build the pass and call **prepare** before this
     * function
     */
    void execute();
    
    void reset();

    ResourceHandle importResource(
        const Util::String& name,
        VulkanAPI::ImageView& imageView,
        const uint32_t width,
        const uint32_t height,
        const vk::Format format,
        const uint8_t samples);

    ResourceHandle moveResource(const ResourceHandle from, const ResourceHandle to);

    std::vector<ResourceBase*>& getResources();
    ResourceBase* getResource(const ResourceHandle handle);
    
    friend class RenderGraphPass;
    friend class RenderGraphBuilder;

private:
    
    ResourceHandle addResource(ResourceBase* resource);
    AttachmentHandle addAttachment(AttachmentInfo& info);
    AttachmentHandle findAttachment(const Util::String& attach);
    
    void initRenderPass();

private:
    
    RGraphContext context;

    // a list of all the render passes
    std::vector<RenderGraphPass> rGraphPasses;
    
    // contains the passes after they have been re-ordered in their optimum sequence
    std::vector<uint32_t> reorderedPasses;
    
    // a virtual list of all the resources associated with this graph
    std::vector<ResourceBase*> resources;

    // used for moving resources - pair = from, to handles
    std::vector<std::pair<ResourceHandle, ResourceHandle>> aliases;

    // The entirety of the attachments for this graph
    std::vector<AttachmentInfo> attachments;
    
};

} // namespace OmegaEngine
