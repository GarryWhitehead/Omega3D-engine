#include "RenderGraph.h"

#include <algorithm>
#include <stdint.h>

namespace OmegaEngine
{

RenderGraphBuilder::RenderGraphBuilder(RenderGraph* rGraph, RenderPass* rPass)
{
	this->rGraph = rGraph;
	this->rPass = rPass;
}

RTargetHandle RenderGraphBuilder::createRenderTarget(Util::String name, Attachment& attach)
{
	RenderTarget rTarget = this->rGraph->addRenderTarget(name, attach);
	rPass->addRenderTarget(rTarget.handle);
	return RTargetHandle(rTarget.index);
}

ResourceHandle RenderGraphBuilder::createTexture(Util::String name, TextureResource* texture)
{
	texture->name = name;
	texture->type = ResourceBase::ResourceType::Texture;
	return rGraph->addResource(texture);
}

ResourceHandle RenderGraphBuilder::createBuffer(Util::String name, BufferResource* buffer)
{
	buffer->name = name;
	buffer->type = ResourceBase::ResourceType::Buffer;
	return rGraph->addResource(buffer);
}

ResourceHandle RenderGraphBuilder::addInput(const ResourceHandle input)
{
	return rPass->addInput(input);
}

ResourceHandle RenderGraphBuilder::addOutput(const ResourceHandle output)
{
	return rPass->addOutput(output);
}

void RenderGraphBuilder::addThreadedExecute(ExecuteFunc&& func)
{
	rPass->addExecute(std::move(func));
}
void RenderPass::addRenderTarget(RTargetHandle& handle)
{
}

ResourceHandle RenderPass::addInput(const ResourceHandle input)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(inputs.begin(), inputs.end());
	if (iter != inputs.end())
	{
		return *iter;
	}
	inputs.emplace_back(input);
	return input;
}

ResourceHandle RenderPass::addOutput(const ResourceHandle output)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(outputs.begin(), outputs.end());
	if (iter != outputs.end())
	{
		return *iter;
	}
	outputs.emplace_back(output);
	return output;
}

void RenderGraph::addExecute(ExecuteFunc&& func)
{
	
}

RenderGraphBuilder RenderGraph::createRenderPass(Util::String name)
{
	// add the pass to the list
	size_t index = renderPasses.size();
	renderPasses.emplace_back(name, index, execute);

	return { this, &renderPasses.back() };
}

RenderTarget& RenderGraph::addRenderTarget(Util::String name, Attachment& descr)
{
	size_t index = renderTargets.size();
	renderTargets.emplace_back(name, descr, index);
	return renderTargets.back();
}

ResourceHandle RenderGraph::addResource(ResourceBase* resource)
{
	size_t index = resources.size();
	resources.emplace_back(resource);
	return index;
}

void RenderGraph::CullResourcsAndPasses(ResourceBase* resource)
{
    // the render pass that uses this resource as an output
    RenderPass* rpass = resource->outputPass;
    
    if (rpass)
    {
        --rpass->outputRef;
        
        // this pass has no more ouput dependencies so can be culled
        if (rpass->outputRef == 0)
        {
            for (ResourceHandle& handle : rpass->inputs)
            {
                ResourceBase* rsrc = resources[handle];
                --resource->inputCount;
                if (rsrc->inputCount == 0)
                {
                    // no input dependencies, so cull too
                    CullResourcsAndPasses(rsrc);
                }
            }
        }
    }
}

void RenderGraph::compile()
{
	
    for (RenderPass& rpass : renderPasses)
	{
        rpass->outputCount = rpass->writes.size();
        
        // work out how many resources are inputs into this pass
		for (ResourceHandle& handle : rpass.inputs)
		{
			ResourceBase* resource = resources[handle];
			++resource->inputCount;
		}

		// and the outputs for this pass
		for (ResourceHandle& handle : rpass.outputs)
		{
			ResourceBase* resource = resources[handle];
			resource->outputPass = &rpass;
		}
	}
    
    // cull passes and reources
    for (ResourceBase* resource : resources)
    {
        // we can't cull resources which are used as inputs for passes
        if (resource->inputCount == 0)
        {
            CullResourcesAndPasses(resource);
        }
    }
    
    // tidy up reference counts - total references, both inputs and outputs
    for (ResourceBase* resource : resources)
    {
        resource->refCount += resource->inputCount;
    }
    
    // bake the resources - this is carried out if this is a newly
    // created resource or non-persistant
    
    
    // Now work out the discard flags for each pass
    
    // And the dependencies
    
    // Finally create the renderpass and framebuffers
    
    // Create the command buffers that will be needed by the pass
    // Command buffers are created each frame and destroyed at the end,
    // whether re-using command buffers could give a performance advantage, this
    // is something maybe to have as an option
}

}    // namespace OmegaEngine
