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

ResourceHandle RenderGraphBuilder::createTexture(const uint32_t width, const uint32_t height, const vk::Format format,
                                                 uint32_t levels, uint32_t layers)
{
	TextureResource* tex = new TextureResource(width, height, format, levels, layers);
	return rGraph->addResource(reinterpret_cast<ResourceBase*>(tex));
}

ResourceHandle RenderGraphBuilder::createBuffer(BufferResource* buffer)
{
	buffer->type = ResourceBase::ResourceType::Buffer;
	return rGraph->addResource(reinterpret_cast<ResourceBase*>(buffer));
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

void RenderGraphBuilder::addExecute(ExecuteFunc&& func, void* renderData, uint32_t secCmdBufferCount)
{
	assert(func);
	assert(renderData);

	rPass->addExecute(std::move(func), renderData, secCmdBufferCount);
}

ResourceHandle RenderGraphPass::addInput(const ResourceHandle input)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find(inputs.begin(), inputs.end(), input);
	if (iter != inputs.end())
	{
		return *iter;
	}
	inputs.emplace_back(input);
	return input;
}

ResourceHandle RenderGraphPass::addOutput(const ResourceHandle output)
{
	// make sure that this handle doesn't already exsist in the list
	// This is just a waste of memory having reduntant resources
	auto iter = std::find_if(outputs.begin(), outputs.end(), output);
	if (iter != outputs.end())
	{
		return *iter;
	}
	outputs.emplace_back(output);
	return output;
}

void RenderGraphPass::prepare(RenderGraphPass* parent)
{
	switch (type)
	{
	case RenderPassType::Graphics:
	{
        // used for signyfing to the subpass the reference ids associated with it
        std::vector<uint32_t> inputRefs, outputRefs;
        
		// if this isn't a merged pass, create a new renderpass. Otherwise, use the parent pass
		if (!parent)
		{
			rpass = new VulkanAPI::RenderPass(Engine.getDevContext());
		}
		else
		{
			assert(parent->rpass);
			rpass = parent->rpass;
		}

		// add the output attachments
		for (ResourceHandle handle : outputs)
		{
			ResourceBase* base = rgraph->resources[handle];
			assert(base->type == ResourceBase::ResourceType::Texture);
			TextureResource* tex = static_cast<TextureResource*>(rgraph->resources[handle]);

			// bake the texture
			if (tex->width != maxWidth || tex->height != maxHeight)
			{
				// not exactly a reason to fatal error maybe, but warn the user
				tex->width = maxWidth;
				tex->height = maxHeight;
				LOGGER_INFO("There appears to be some discrepancy between this passes resource dimensions\n");
			}
            
            outputRefs.emplace_back(tex->referenceId);
			tex->bake();

			// add a attachment
			rpass->addOutputAttachment(tex->format, tex->initialLayout, tex->finalLayout, tex->clearFlags);

			// add the reference
			rpass->addOutputRef(tex->referenceId);
		}

		// input attachments
		for (ResourceHandle handle : inputs)
		{
			ResourceBase* base = rgraph->resources[handle];
			assert(base->type == ResourceBase::ResourceType::Texture);
			TextureResource* tex = static_cast<TextureResource*>(rgraph->resources[handle]);
            
            inputRefs.emplace_back(tex->referenceId);
			rpass->addInputRef(tex->referenceId);
		}

		// Add a subpass. If this is a merged pass, then this will be added to the parent
		rpass->addSubPass(inputRefs, outputRefs);
		rpass->addSubpassDependency(subpass.depFlags);
		break;
	}
	case RenderPassType::Compute:
	{
		break;
	}
	}
}

void RenderGraphPass::bake()
{
	// create the renderpass
	rpass->prepare();
}

void RenderGraphPass::addExecute(ExecuteFunc&& func, void* userData, uint32_t threadCount)
{
	execute = ExecuteInfo{ func, userData, threadCount };
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
	// the render pass that outputs to this resource
	RenderGraphPass* rpass = resource->outputPass;

	if (rpass)
	{
		--rpass->refCount;

		// this pass has no more write attahment dependencies so can be culled
		if (rpass->refCount == 0)
		{
			for (ResourceHandle& handle : rpass->inputs)
			{
				ResourceBase* rsrc = resources[handle];
				--resource->inputCount;
				if (rsrc->inputCount == 0)
				{
					// no input dependencies, so see if we can cull this pass
					CullResourcesAndPasses(rsrc);
				}
			}
		}
	}
}

AttachmentHandle RenderGraph::addAttachment(AttachmentInfo& info)
{
}

bool RenderGraph::compile()
{

	for (RenderGraphPass& rpass : renderPasses)
	{
		if (rpass.type == RenderGraphPass::RenderPassType::Graphics)
		{
			// the number of resources this pass references too
			rpass.refCount = rpass.outputs.size();

			// work out how many resources read from this resource
			for (ResourceHandle& handle : rpass.inputs)
			{
				ResourceBase* resource = resources[handle];
				++resource->inputCount;
			}

			// for the outputs, set the pass which writes to this resource
			for (ResourceHandle& handle : rpass.outputs)
			{
				ResourceBase* resource = resources[handle];
				resource->outputPass = &rpass;
			}
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

	// finialise the attachments
	size_t passCount = renderPasses.size();
	for (size_t i = 0; i < passCount; ++i)
	{
		RenderGraphPass& rpass = renderPasses[i];

		// passes with no refences are treated as culled
		if (rpass.refCount == 0)
		{
			continue;
		}

		uint32_t refId = 0;
		uint32_t maxWidth, maxHeight;
		for (const ResourceHandle handle : rpass.outputs)
		{
			ResourceBase* base = resources[handle];
			if (base->type == ResourceBase::ResourceType::Texture)
			{
				TextureResource* tex = reinterpret_cast<TextureResource*>(base);

				// used by the attachment descriptor
				tex->referenceId = refId++;

				// use the resource with max dimensions - they should all be identical really
				maxWidth = std::max(maxWidth, tex->width);
				maxHeight = std::max(maxHeight, tex->height);
			}
		}

		// if the outputs from this pass are used as inputs in another pass, we can probably merge
		// we create a linked list of passes - wth the fact that the parent isn't nullptr denoting that 
		// these are merged. The ref count of merged passes, except for the parent is set to zero
		// to ensure that the passes that are merged aren't also created seperately
		// TODO: check - there maybe other circumstances in which a pass can not be merged
		if (i < passCount - 1)
		{
			RenderGraphPass& nextPass = renderPasses[i + 1];
			if (!nextPass.inputs.empty())
			{
				rpass.childMergePass = &nextPass;
				rpass.refCount = 0;
			}
			else
			{
				// if the previous pass was a merge - then this will be the final pass in the merge
				RenderGraphPass& prevPass = renderPasses[i - 1];
				if (prevPass.childMergePass)
				{
					rpass.refCount = 0;
				}
			}
			
		}
	}

	// Now work out the discard flags for each pass

	// And the dependencies
}

void RenderGraph::initRenderPass()
{
	for (RenderGraphPass& rpass : renderPasses)
	{
		if (rpass.refCount == 0)
		{
			continue;
		}

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

			// check if this is merged - will have child passes associated with it
			if (rpass.childMergePass)
			{
				// prepare this pass first
				rpass.prepare();

				// add each child pass info to this parent - creating a multi-pass
				RenderGraphPass* child = &rpass;
				while (!child->childMergePass)
				{
					child.prepare(rpass);
					child = child->childMergePass;
				}
				rpass.bake();
			}
			else
			{
				// create the renderpass
				rpass.bake();
			}

			// create the framebuffer - this is linked to the renderpass
			rpass.framebuffer->prepare(rpass, views, rpass.width, rpass.height);

			// and the command buffer - this is linked to both the pass and frame buffer
			cbManager->newInstance(QueueType::Graphics);

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

AttachmentHandle RenderGraph::findAttachment(Util::String req)
{
	uint64_t index = 0;
	for (AttachmentInfo& attach : attachments)
	{
		if (attach.name.compare(req))
		{
			return index;
		}
		++index;
	}
	return UINT64_MAX;
}

}    // namespace OmegaEngine
