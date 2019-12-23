#pragma once

#include "RenderGraph/Resources.h"

#include "VulkanAPI/CommandBufferManager.h"
#include "VulkanAPI/RenderPass.h"

#include "utility/CString.h"
#include "utility/BitSetEnum.h"

#include "OEMaths/OEMaths.h"

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
}    // namespace VulkanAPI

namespace OmegaEngine
{

// forward declerations
class RenderGraph;
class Renderer;
class RenderGraphPass;

/**
* @brief A useful container for grouping together render graph variables for use externally
*/
struct RGraphContext
{
	// the command buffer handle for this pass
	VulkanAPI::CmdBuffer* cmdBuffer = nullptr;

	// the vulkan render pass for this pass
	VulkanAPI::RenderPass* rpass = nullptr;

	// useful vulkan managers
	VulkanAPI::CmdBufferManager* cbManager = nullptr;
	VulkanAPI::VkDriver* driver = nullptr;

	// keep track of the renderer
	Renderer* renderer = nullptr;
};

using ExecuteFunc = std::function<void(RGraphContext&)>;

class RenderGraphPass
{
public:
	// At the moment, vulkan doesn't support compute subpasses. Thus,
	// if a compute stage is required. Then the renderpass will end, and the
	// compute pass will be deployed, and any remaining graphic passes will be
	// started in another pass after the compute finishes. This isn't ideal performance wise, as switching
	// between different pipelines is expensive, so compute calls should be batched, ideally before the the graphics renderpass
	enum Type
	{
		Graphics,
		Compute
	};
    
    RenderGraphPass() = default;
    RenderGraphPass(Util::String name, const Type type, RenderGraph& rGaph);
    
	// not copyable
	RenderGraphPass(const RenderGraphPass&) = delete;
	RenderGraphPass& operator=(const RenderGraphPass&) = delete;

	// adds a input attachment reader handle to the pass
	ResourceHandle addInput(ResourceHandle input);

	// adds a colour/depth/stencil attachment writer to the pass
	ResourceHandle addOutput(ResourceHandle output);

	// A callback function which will be called each frame
	void addExecute(ExecuteFunc&& func);

	// init the vulkan renderpass - attachments, ref, dependencies
	// these can be added to a parent merged pass if defined
	void prepare(VulkanAPI::VkDriver& driver, RenderGraphPass* parent = nullptr);

	// creates the vulkan renderpass. You must call **prepare()** first
	void bake();

	// Sets the clear colour for all attachments for this pass	
	void setClearColour(OEMaths::colour4& clearCol);

	// sets the depth clear for this pass
	void setDepthClear(float depthClear);

	friend class RenderGraph;

private:
    
	RenderGraph& rGraph;
    Util::String name;
	Type type;

	// a list of handles of input and output attachments
	std::vector<ResourceHandle> inputs;     // input attachments
	std::vector<ResourceHandle> outputs;    // colour/depth/stencil attachments

	// the execute function to be used by this pass
	ExecuteFunc execFunc;

	// ====== compiler set =========
	// reference count for the number of outputs
	size_t refCount = 0;

	// the max dimesnions of the resource this pass outputs too.
	uint32_t maxWidth = 0;
	uint32_t maxHeight = 0;

	// If this pass is mergeable, then this will point to a linked list of mergable passes
	RenderGraphPass* nextPass = nullptr;
    
    // This is only used if this pass will be used threaded, i.e. using secondary cmd buffers. If this is the case, all cmd buffers will be allocated from this pool and will be reset per frame through a call to **update**.
    VulkanAPI::CmdPool* cmdPool = nullptr;
    
    Util::BitSetEnum<VulkanAPI::SubpassFlags> flags;
    
	// ======= vulkan specific ================
	// Kept in a struct as this will be passed around when rendering passes
	RGraphContext context;

	VulkanAPI::FrameBuffer framebuffer;

	// clear colours for this pass
	OEMaths::colour4 clearCol = { 0.0f };
	float depthClear = 1.0f;
};

/**
* @brief Useful helper functions for building the rendergraph
*/
class RenderGraphBuilder
{
public:
	RenderGraphBuilder(RenderGraph* rGraph, RenderGraphPass* rPass);

	/**
	* @ creates a texture resource for using as a render target in a graphics  pass
	*/
    ResourceHandle createTexture(const uint32_t width, const uint32_t height,
            const vk::Format format, uint32_t levels = 1, uint32_t layers = 1);

	/**
    * @ creates a buffer resource for using as a render target in a compute pass
    */
    ResourceHandle createBuffer(BufferResource* buffer);

	/**
	* @brief Adds a input attachment to the render pass. There must be a corresponding output attachment otherwise returns UINT64_MAX as error.
	*/
	AttachmentHandle addInputAttachment(Util::String name);

	/**
	* @brief Adds a output attachment such as a colour/depth/stencil attachment to the pass
	*/
	AttachmentHandle addOutputAttachment(Util::String name, const ResourceHandle resource);

	/**
     * @brief Adds a function to execute each frame for this renderpass 
	 * @param func The function to execute. Must be of the format (void*)(RenderPassContext&)
     *  
     */
	void addExecute(ExecuteFunc&& func);

	/**
	* @brief Sets the clear colour for all attachments for this pass
	*/
	void setClearColour(OEMaths::colour4& clearCol);

	/**
	* @brief Sets the depth clear for this pass
	*/
	void setDepthClear(float depthClear);

private:
	// a reference to the graph and pass we are building
	RenderGraph* rGraph = nullptr;
	RenderGraphPass* rPass = nullptr;
};

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
	* @ return A convience render graph builder which is used by the user to create the defined pass
	*/
	RenderGraphBuilder createRenderPass(Util::String name, const RenderGraphPass::Type type);

	/**
	* @brief Takes the user-defined graph and builds the render pass.
	* The following procedures are carried out:
	* 1. optimisation of the graph and compilation
	* 2. init render passes, framebuffers and command buffers for each pass
	* 3. 
	*/
	void prepare();

	/**
	* The execution of the render pass. You must build the pass and call **prepare** before this function
	*/
	void execute();
    
    // ============== getters ==================
    std::vector<ResourceBase*>& getResources();
    
    friend class RenderGraphPass;
    friend class RenderGraphBuilder;
    
private:
    
	void CullResourcesAndPasses(ResourceBase* resource);
	ResourceHandle addResource(ResourceBase* resource);
	AttachmentHandle addAttachment(AttachmentInfo& info);
	AttachmentHandle findAttachment(Util::String attach);

	void initRenderPass();

	// optimises the render graph if possible and fills in all the blanks - i.e. referneces, flags, etc.
	bool compile();
    
private:
    
	VulkanAPI::VkDriver& driver;

	// a list of all the render passes
	std::vector<RenderGraphPass> renderPasses;

	// a virtual list of all the resources associated with this graph
	std::vector<ResourceBase*> resources;

	// The entirety of the attachments for this graph
	std::vector<AttachmentInfo> attachments;
};

}    // namespace OmegaEngine
