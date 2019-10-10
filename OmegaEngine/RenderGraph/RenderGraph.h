#pragma once

#include "RenderGraph/Resources.h"

#include "utility/String.h"

#include <functional>
#include <vector>

namespace OmegaEngine
{

// forward decleration
class RenderGraph;

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

class RenderPass
{
public:

	void addRenderTarget(RTargetHandle& handle);

	ResourceHandle addInput(ResourceHandle input);

	ResourceHandle addOutput(ResourceHandle output);

private:

    RenderGraph* rgraph = nullptr;
    
	std::vector<RTargetHandle> targetHandles;

	std::vector<ResourceHandle> inputs;
	std::vector<ResourceHandle> outputs;
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
	* @ brief Creates a new render target and assigns the attachment
	*/
	RTargetHandle createRenderTarget(Util::String name, Attachment& decsr);

	/**
	* @ creates a new virtual texture based on the user definitions
	*/
	ResourceHandle createTexture(Util::String name, TextureResource* descr);

	/**
	* @brief Creates a buffer resource target
	*/
	ResourceHandle createBuffer(Util::String name, BufferResource* buffer);

	/**
	* @brief Adds a input such as a texture resource to the render target
	*/
	ResourceHandle addInput(const ResourceHandle input);

	/**
	* @brief Adds a output such as a texture resource to the render target
	*/
	ResourceHandle addOutput(const ResourceHandle output);
    
    /**
     * @brief Adds the execution lamda to the renderpass - this will be executed each frame
     *      on its own thread using secondary command buffers
     */
    void addThreadedExecute();

private:
	// a reference to the graph and pass we are building
	RenderGraph* rGraph = nullptr;
	RenderPass* rPass = nullptr;
};

class RenderGraph
{
public:
	using ExecuteFunc = std::function<void(void*)>;

	/**
	* @brief Creates a new renderpass. 
	* @param name The name of this pass
	*/
	RenderGraphBuilder& createRenderPass(Util::String name);

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
	// all of the render targets accociated with this graph
	std::vector<RenderTarget> renderTargets;

	// a list of all the render passes
	std::vector<RenderPass> renderPasses;

	// a virtual list of all the resources associated with this graph
	std::vector<ResourceBase> resources;
};

}    // namespace OmegaEngine
