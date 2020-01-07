#include "RenderGraph.h"

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/VkDriver.h"

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
		LOGGER_ERROR("Unable to find corresponding output attachment whilst trying to add input attachment.");
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

void RenderGraphBuilder::addExecute(ExecuteFunc&& func)
{
	assert(func);
	rPass->addExecute(std::move(func));
}

void RenderGraphBuilder::setClearColour(OEMaths::colour4& clearCol)
{
	rPass->setClearColour(clearCol);
}

void RenderGraphBuilder::setDepthClear(float depthClear)
{
	rPass->setDepthClear(depthClear);
}

// ============================== render graph pass =================================

RenderGraphPass::RenderGraphPass(Util::String name, const Type type, RenderGraph& rGraph) :
    rGraph(rGraph),
    name(name),
    type(type)
{
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
	auto iter = std::find(outputs.begin(), outputs.end(), output);
	if (iter != outputs.end())
	{
		return *iter;
	}
	outputs.emplace_back(output);
	return output;
}

void RenderGraphPass::prepare(VulkanAPI::VkDriver& driver, RenderGraphPass* parent)
{
	switch (type)
	{
	case Type::Graphics:
	{
		// used for signyfing to the subpass the reference ids associated with it
		std::vector<uint32_t> inputRefs, outputRefs;

		// if this isn't a merged pass, create a new renderpass. Otherwise, use the parent pass
        if (!flags.testBit(VulkanAPI::SubpassFlags::Merged))
		{
			context.rpass = new VulkanAPI::RenderPass(driver.getContext());
		}
		else
		{
			assert(parent->context.rpass);
			context.rpass = parent->context.rpass;
		}

        auto& resources = rGraph.getResources();
        
		// add the output attachments
		for (ResourceHandle handle : outputs)
		{
            ResourceBase* base = resources[handle];
			assert(base->type == ResourceBase::ResourceType::Texture);
			TextureResource* tex = static_cast<TextureResource*>(resources[handle]);
          
			// bake the texture
			if (tex->width != maxWidth || tex->height != maxHeight)
			{
				// not exactly a reason to fatal error maybe, but warn the user
                // maybe do a blit here instead? The answer is yes, TODO!!!!!
				tex->width = maxWidth;
				tex->height = maxHeight;
				LOGGER_WARN("There appears to be some discrepancy between this passes resource dimensions\n");
			}

			outputRefs.emplace_back(tex->referenceId);
            tex->bake(driver);

			// add a attachment
            context.rpass->addOutputAttachment(tex->format, tex->referenceId, tex->clearFlags, tex->samples);
		}

		// input attachments
		for (ResourceHandle handle : inputs)
		{
            ResourceBase* base = resources[handle];
			assert(base->type == ResourceBase::ResourceType::Texture);
			TextureResource* tex = static_cast<TextureResource*>(resources[handle]);
            
            // for clear flags, we always 'store' for the loadOp
            // check the format here, if stencil set the stencil Op flags??
            tex->clearFlags.attachLoad = VulkanAPI::RenderPass::LoadType::Store;
            
			inputRefs.emplace_back(tex->referenceId);
			context.rpass->addInputRef(tex->referenceId);
		}

		// Add a subpass. If this is a merged pass, then this will be added to the parent
		context.rpass->addSubPass(inputRefs, outputRefs);
        context.rpass->addSubpassDependency(parent->flags);
        
		break;
	}
	case Type::Compute:
	{
		break;
	}
	}
}

void RenderGraphPass::bake()
{
	// create the renderpass
	context.rpass->prepare();
}

void RenderGraphPass::addExecute(ExecuteFunc&& func)
{
	execFunc = std::move(func);
}

void RenderGraphPass::setClearColour(OEMaths::colour4& colour)
{
	context.clearCol = colour;
}

void RenderGraphPass::setDepthClear(float depth)
{
	context.depthClear = depth;
}

// =========================== RenderGraph ===============================

RenderGraph::RenderGraph(VulkanAPI::VkDriver& driver)
    : driver(driver)
{
}

RenderGraph::~RenderGraph()
{
}

RenderGraphBuilder RenderGraph::createRenderPass(Util::String name, const RenderGraphPass::Type type)
{
	// add the pass to the list
    renderPasses.push_back(RenderGraphPass(name, type, *this));
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
	return 0;
}

bool RenderGraph::compile()
{

	for (RenderGraphPass& rpass : renderPasses)
	{
		if (rpass.type == RenderGraphPass::Type::Graphics)
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
    
    // fill some of the subpass dependency flags
    renderPasses[0].flags |= VulkanAPI::SubpassFlags::TopOfPipeline;
    renderPasses[passCount - 1].flags |= VulkanAPI::SubpassFlags::BottomOfPipeline;
    
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
                
                // if the texture format is depth/stencil, then set as depth/stencil read,
                // otherwise, colour read/write
                if (tex->isDepthFormat() || tex->isStencilFormat())
                {
                    if (tex->isDepthFormat())
                    {
                        rpass.flags |= VulkanAPI::SubpassFlags::DepthRead;
                    }
                    if (tex->isStencilFormat())
                    {
                        rpass.flags |= VulkanAPI::SubpassFlags::StencilRead;
                    }
                }
                else
                {
                    // assume must be a colour format
                    rpass.flags |= VulkanAPI::SubpassFlags::ColourRead;
                }
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
            
            // create a linked list
            rpass.nextPass = &nextPass;
            
			if (!nextPass.inputs.empty())
			{
                rpass.flags |= VulkanAPI::SubpassFlags::Merged;
				rpass.refCount = 0;
			}
			else
			{
				// if the previous pass was a merge - then this will be the final pass in the merge
				RenderGraphPass& prevPass = renderPasses[i - 1];
				if (prevPass.flags.testBit(VulkanAPI::SubpassFlags::Merged))
				{
					rpass.refCount = 0;
				}
			}
		}
	}
    return true;
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
		case RenderGraphPass::Type::Graphics:
		{
			std::vector<VulkanAPI::ImageView*> views(rpass.outputs.size());
			for (size_t i = 0; rpass.outputs.size(); ++i)
			{
				AttachmentHandle handle = rpass.outputs[i];
				assert(handle < attachments.size());
				AttachmentInfo& attach = attachments[handle];

				// bake the resources
                VulkanAPI::ImageView* view = reinterpret_cast<VulkanAPI::ImageView*>(attach.bake(driver, *this));
                views.emplace_back(view);
			}

			// check if this is merged - will have child passes associated with it
			if (rpass.flags.testBit(VulkanAPI::SubpassFlags::Merged))
			{
				// prepare this pass first
				rpass.prepare(driver);

				// add each child pass info to this parent - creating a multi-pass
				RenderGraphPass* child = &rpass;
				while (!child->flags.testBit(VulkanAPI::SubpassFlags::Merged))
				{
					child->prepare(driver, &rpass);
					child = child->nextPass;
				}
				rpass.bake();
			}
			else
			{
				// create the renderpass
				rpass.bake();
			}

			// create the framebuffer - this is linked to the renderpass
            rpass.context.framebuffer.prepare(*rpass.context.rpass, views, rpass.maxWidth, rpass.maxHeight, 1);

            VulkanAPI::CmdPool* cmdPool = rpass.context.cbManager->getMainPool();
            rpass.context.cmdBuffer = cmdPool->createPrimaryCmdBuffer(&driver.getCbManager());
			break;
		}

		case RenderGraphPass::Type::Compute:
		{
			// TODO
            break;
		}
		}
	}
}

void RenderGraph::prepare()
{
	// start by optimising the graph and filling out the structure
	compile();

	// init the renderpass resources - command buffer, frame buffers, etc.
	initRenderPass();
}

void RenderGraph::execute()
{
	// iterate over all passes and execute the registered callback function
	for (RenderGraphPass& rpass : renderPasses)
	{
		// start the render pass
		VulkanAPI::CmdBufferManager& manager = driver.getCbManager();
        
        assert(rpass.context.cmdBuffer);
		manager.beginRenderpass(rpass.context.cmdBuffer, *rpass.context.rpass, rpass.context.framebuffer);

		rpass.execFunc(rpass.context);
	}
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

std::vector<ResourceBase*>& RenderGraph::getResources()
{
    return resources;
}

ResourceBase* RenderGraph::getResource(const ResourceHandle handle)
{
    return resources[handle];
}

}    // namespace OmegaEngine
