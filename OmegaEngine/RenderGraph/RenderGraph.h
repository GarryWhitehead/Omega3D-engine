#pragma once

#include "RenderGraph/Resources.h"

#include "utility/String.h"

#include <functional>
#include <vector>

// forward decleartion
namespace VulkanAPI
{
class CommandBuffer;
}

namespace OmegaEngine
{

// forward decleration
class RenderGraph;
class RenderingInfo;

struct RGraphContext
{
	VulkanAPI::CommandBuffer* cmdBuffer = nullptr;
};

class RTargetHandle
{
public:
	RTargetHandle(const uint64_t h)
	{
		handle = h;
	}

private:
	uint64_t handle;
};

using ExecuteFunc = std::function<void(RenderingInfo&, RGraphContext&)>;

class RenderPass
{
public:

	void addRenderTarget(RTargetHandle& handle);

    // adds a input attachment reader handle to the pass
	ResourceHandle addRead(ResourceHandle input);
    
    // adds a colour/depth/stencil attachment writer to the pass
	ResourceHandle addWrite(ResourceHandle output);

    // A callback function which will be called each frame
	void addExecute(ExecuteFunc&& func);

	friend class RenderGraph;

private:

    RenderGraph* rgraph = nullptr;
    
	// a list of taergets associated with this pass.
	std::vector<RTargetHandle> targets;

	// a list of handles of input and output attachments
	std::vector<ResourceHandle> readers;    // input attachments
	std::vector<ResourceHandle> writers;    // colour/depth/stencil attachments

	// the eecute function to be used by this pass
	ExecuteFunc executeFunc = nullptr;
    
    // compiler set.....
    // reference count for the number of outputs
    size_t outputRef = 0;

};

struct RenderTarget
{
	Util::String name;

	uint64_t rPassIndex = 0;

	// the target descriptor
	std::vector<Attachment> attachments;
};

/**
* @brief Useful helper functions for building the rendergraph
*/
class RenderGraphBuilder
{
public:
	RenderGraphBuilder(RenderGraph* rGraph, RenderPass* rPass);

	/**
	* @ creates a texture resource for using as a render target in a graphics  pass
	*/
    ResourceHandle RenderGraphBuilder::createTexture(Util::String name, ResourceBase* texture);
    
    /**
    * @ creates a buffer resource for using as a render target in a compute pass
    */
    ResourceHandle RenderGraphBuilder::createBuffer(Util::String name, ResourceBase* buffer);

	/**
	* @brief Adds a input attachment to the render pass. A resource must be created beforehand by callin either **createTexture**
     * for a graphics pipeline or **createBuffer** for a compute pipeline
	*/
	AttachmentHandle addInputAttachment(Util::String name, const ResourceHandle resource);

	/**
	* @brief Adds a output attachment such as a colour/depth/stencil attachment to the pass
	*/
    AttachmentHandle addOutputAttachment(Util::String name, const ResourceHandle resource);
    
    /**
     * @brief Adds the execution lamda to the renderpass - this will be executed each frame
     *      on its own thread using secondary command buffers
     */
    void addThreadedExecute(ExecuteFunc&& func);

private:
	// a reference to the graph and pass we are building
	RenderGraph* rGraph = nullptr;
	RenderPass* rPass = nullptr;
};

class RenderGraph
{
public:
	

	/**
	* @brief Creates a new renderpass. 
	* @param name The name of this pass
	*/
	RenderGraphBuilder createRenderPass(Util::String name);

	/**
	* @brief Create a new instance of a render target based on the supplied descriptor
	* @param name The name of this target. Will be used to reference
	* @param descr A descriptor to use for this target
	* @return A reference to the newly created target
	*/
	RenderTarget& addRenderTarget(Util::String name, Attachment& descr);

	/**
	* @brief Creates a new target resource
	*/
	ResourceHandle addResource(ResourceBase* resource);

	void compile();

	void execute();

private:
    
    void CullResourcsAndPasses(ResourceBase* resource);
    
private:
	// all of the render targets accociated with this graph
	std::vector<RenderTarget> renderTargets;

	// a list of all the render passes
	std::vector<RenderPass> renderPasses;

	// a virtual list of all the resources associated with this graph
	std::vector<ResourceBase*> resources;
};

}    // namespace OmegaEngine
