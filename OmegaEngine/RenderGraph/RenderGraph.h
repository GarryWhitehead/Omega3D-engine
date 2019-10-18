#pragma once

#include "RenderGraph/Resources.h"

#include "utility/String.h"

#include <functional>
#include <vector>

// forward decleartion
namespace VulkanAPI
{
class CommandBuffer;
class FrameBuffer;
class RenderPass;
}    // namespace VulkanAPI

namespace OmegaEngine
{

// forward decleration
class RenderGraph;
class RenderGraphPass;

struct RGraphContext
{
	VulkanAPI::CommandBuffer* cmdBuffer = nullptr;
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
	enum class RenderPassType
	{
		Graphics,
		Compute
	};

	// not copyable
	RenderGraphPass(const RenderGraphPass&) = delete;
	RenderGraphPass& operator=(const RenderGraphPass&) = delete;

	// adds a input attachment reader handle to the pass
	ResourceHandle addInput(ResourceHandle input);

	// adds a colour/depth/stencil attachment writer to the pass
	ResourceHandle addOutput(ResourceHandle output);

	// A callback function which will be called each frame
	void addExecute(ExecuteFunc&& func, void* userData, uint32_t threadCount);

	// creates the vulkan renderpass
	void bake();

	friend class RenderGraph;

private:
	struct ExecuteInfo
	{
		ExecuteFunc func;
		void* data = nullptr;
		uint32_t threads = 0;
	};

private:
	RenderGraph* rgraph = nullptr;

	RenderPassType type;

	// a list of handles of input and output attachments
	std::vector<ResourceHandle> inputs;     // input attachments
	std::vector<ResourceHandle> outputs;    // colour/depth/stencil attachments
	
	// the execute function to be used by this pass
	ExecuteInfo execute;

	// ====== compiler set =========
	// reference count for the number of outputs
	size_t refCount = 0;

	// the max dimesnions of the resource this pass outputs too.
	uint32_t maxWidth = 0;
	uint32_t maxHeight = 0;

	// If this pass is mergeable, then this will point to a linked list of mergable passes
	RenderGraphPass* childMergePass = nullptr;

	// ======= vulkan specific ================
	VulkanAPI::CommandBuffer* cmdBuffer = nullptr;
	VulkanAPI::RenderPass* rpass = nullptr;

	// Renderpasses can have more than one frame buffer - if triple buffered for exmample
	std::vector<VulkanAPI::FrameBuffer*> framebuffer;
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
	ResourceHandle RenderGraphBuilder::createTexture(const uint32_t width, const uint32_t height,
	                                                 const vk::Format format, uint32_t levels = 1, uint32_t layers = 1);

	/**
    * @ creates a buffer resource for using as a render target in a compute pass
    */
	ResourceHandle RenderGraphBuilder::createBuffer(BufferResource* buffer);

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
	 * @param renderData The data for the execution function
	 * @param secCmdBufferCount If this is greater than zero, then the data will be sliced into data chunks and executed on seperate threads
     *  
     */
	void addExecute(ExecuteFunc&& func, void* renderData, uint32_t secCmdBufferCount = 0);

private:
	// a reference to the graph and pass we are building
	RenderGraph* rGraph = nullptr;
	RenderGraphPass* rPass = nullptr;
};

class RenderGraph
{
public:
	/**
	* @brief Creates a new renderpass. 
	* @param name The name of this pass
	* @ return A convience render graph builder which is used by the user to create the defined pass
	*/
	RenderGraphBuilder createRenderPass(Util::String name);

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

	friend class RenderGraphBuilder;
	friend class RenderGraphPass;

private:
	void CullResourcesAndPasses(ResourceBase* resource);

	// Creates a new target resource
	ResourceHandle addResource(ResourceBase* resource);

	// adds a new attachment to the graph
	AttachmentHandle addAttachment(AttachmentInfo& info);

	// finds an attachment by name and returns its handle. Returns UNIT64_MAX if invalid name.
	AttachmentHandle findAttachment(Util::String attach);

	void initRenderPass();

	// optimises the render graph if possible and fills in all the blanks - i.e. referneces, flags, etc.
	bool compile();

private:
	// a list of all the render passes
	std::vector<RenderGraphPass> renderPasses;

	// a virtual list of all the resources associated with this graph
	std::vector<ResourceBase*> resources;

	// The entirety of the attachments for this graph
	std::vector<AttachmentInfo> attachments;
};

}    // namespace OmegaEngine
