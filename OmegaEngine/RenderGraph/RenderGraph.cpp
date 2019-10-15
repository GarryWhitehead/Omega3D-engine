#include "RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/RenderPass.h"

#include "utility/Logger.h"

#include <algorithm>
#include <assert.h>
#include <stdint.h>

namespace OmegaEngine
{

RenderGraphBuilder::RenderGraphBuilder(RenderGraph* rGraph, RenderGraphPass* rPass)
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

AttachmentHandle RenderGraphBuilder::addInputAttachment(Util::String name)
{
    // link the input with the outputted resource
    AttachmentHandle handle = rGraph->findAttachment(name);
    if (handle == UINT64_MAX)
    {
        LOGGER_ERROR("Unable to find corresponding output attachment.");
        return UINT64_MAX;
    }
    
	rPass->addInput(handle);
	return handle;
}

AttachmentHandle RenderGraphBuilder::addOutputAttachment(Util::String name, const ResourceHandle resource)
{
	AttachmentInfo info;
	info.name = name;
	info.resource = resource;

	AttachmentHandle handle = rGraph->addAttachment(info);
	rPass->addOutput(handle);
	return handle;
}

ResourceHandle RenderGraphPass::addInput(const ResourceHandle read)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(inputs.begin(), inputs.end());
	if (iter != inputs.end())
	{
		return *iter;
	}
	inputs.emplace_back(read);
	return read;
}

ResourceHandle RenderGraphPass::addOutput(const ResourceHandle write)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(outputs.begin(), outputs.end());
	if (iter != outputs.end())
	{
		return *iter;
	}
	outputs.emplace_back(write);
	return write;
}

void RenderGraphPass::bake()
{
	switch (type)
	{
	case RenderPassType::Graphics:
	{
    
        // add the output attachments
		for (AttachmentHandle handle : outputs)
		{
			AttachmentInfo attach = rgraph->attachments[handle];
            TextureResource* tex = static_cast<TextureResource*>(rgraph->resources[attach.resource]);
            
            // add a colour attachment
            renderpass->addAttachment(tex->format, tex->initialLayout, tex->finalLayout, tex->clearFlags);
            
            // add the reference
			renderpass->addInputRef(tex->referenceId);
		}

		// input attachments
		for (AttachmentHandle handle : inputs)
		{
			AttachmentInfo attach = rgraph->attachments[handle];
			BaseResource* tex = rgraph->resources[attach.resource];
			renderpass->addOutputRef(tex->reference);
		}

		for (auto& subpass : subpasses)
		{
			renderpass->addSubPass(subpass.inputRefs, subpass.outputRefs);
			renderpass->addSubpassDependency(subpass.dependency);
		}

		// finally create the renderpass
		renderpass->prepare();
		break;
	}
	case RenderPassType::Compute:
	{
		break;
	}
	}
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

ResourceHandle RenderGraph::addResource(ResourceBase* resource)
{
	size_t index = resources.size();
	resources.emplace_back(resource);
	return index;
}

void RenderGraph::CullResourcesAndPasses(ResourceBase* resource)
{
	// the render pass that uses this resource as an output
	RenderGraphPass* rpass = resource->outputPass;

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
					CullResourcesAndPasses(rsrc);
				}
			}
		}
	}
}

AttachmentHandle RenderGraph::addAttachment(AttachmentInfo& info)
{
}

void RenderGraph::compile()
{

	for (RenderGraphPass& rpass : renderPasses)
	{
		rpass.reference = rpass.outputs.size();

		// work out how many resources are input attachments into this pass
		for (ResourceHandle& handle : rpass.inputs)
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

	// Now work out the discard flags for each pass

	// And the dependencies
}

void RenderGraph::initRenderPass()
{
	for (RenderGraphPass& rpass : renderPasses)
	{
		switch (rpass.type)
		{
		case RenderGraphPass::RenderPassType::Graphics:
		{
			std::vector<VulkanAPI::ImageView> views(rpass.outputs.size());
			for (size_t i = 0; rpass.outputs.size(); ++i)
			{
				AttachmentHandle handle = rpass.outputs[i];
				assert(handle < attachments.size());
				AttachmentInfo& attach = attachments[handle];

				// bake the resources
				views[i] = attach.bake();
			}

			// create the renderpass
			rpass.bake();

			// create the framebuffer - this is linked to the renderpass
			rpass.framebuffer->prepare(rpass, views, rpass.width, rpass.height);

			// and the command buffer - this is linked to both the pass and frame buffer
			rpass.cmdBuffer->prepare();

			// also setup secondary buffers if needed

			break;
		}

		case RenderGraphPass::RenderPassType::Compute:
		{
			break;
		}
		}
	}
}

void RenderGraph::prepare()
{
	// start by optimising the graph and fillimng out the structure
	compile();

	// init the renderpass resources - command buffer, frame buffers, etc.
	initRenderPass();
}

void RenderGraph::execute()
{
    
}

AttachmentHandle RenderGraph::findAttachment(Util::String attach)
{
    
}

}    // namespace OmegaEngine
