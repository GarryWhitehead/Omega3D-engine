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

ResourceHandle RenderGraphBuilder::createTexture(Util::String name, ResourceBase* texture)
{
	texture->name = name;
	texture->type = ResourceBase::ResourceType::Texture;
	return rGraph->addResource(texture);
}

ResourceHandle RenderGraphBuilder::createBuffer(Util::String name, ResourceBase* buffer)
{
	buffer->name = name;
	buffer->type = ResourceBase::ResourceType::Buffer;
	return rGraph->addResource(buffer);
}

AttachmentHandle RenderGraphBuilder::addInputAttachment(Util::String name, const ResourceHandle resource)
{
    AttachmentInfo info;
    info.name = name;
    info.resource = resource;
    
    AttachmentHandle handle = rgraph->addAttachment(info);
    rPass->addInput(handle);
    return handle;
}

AttachmentHandle RenderGraphBuilder::addOutputAttachment(Util::String name, const ResourceHandle resource)
{
    AttachmentInfo info;
    info.name = name;
    info.resource = resource;
    
    AttachmentHandle handle = rgraph->addAttachment(info);
    rPass->addOutput(handle);
    return handle;
}



ResourceHandle RenderPass::addRead(const ResourceHandle read)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(readers.begin(), readers.end());
	if (iter != inputs.end())
	{
		return *iter;
	}
	readers.emplace_back(read);
	return read;
}

ResourceHandle RenderPass::addWrite(const ResourceHandle write)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(writers.begin(), writers.end());
	if (iter != writers.end())
	{
		return *iter;
	}
	writers.emplace_back(write);
	return write;
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
        --rpass->writeRef;
        
        // this pass has no more write attahment dependencies so can be culled
        if (rpass->writeRef == 0)
        {
            for (ResourceHandle& handle : rpass->readers)
            {
                ResourceBase* rsrc = resources[handle];
                --resource->readCount;
                if (rsrc->readCount == 0)
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
        rpass->outputCount = rpass->writers.size();
        
        // work out how many resources are input attachments into this pass
		for (ResourceHandle& handle : rpass.readers)
		{
			ResourceBase* resource = resources[handle];
			++resource->readCount;
		}

		// and the outputs for this pass
		for (ResourceHandle& handle : rpass.writers)
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
