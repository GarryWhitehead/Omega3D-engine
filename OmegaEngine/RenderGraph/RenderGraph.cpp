#include "RenderGraph.h"

#include <stdint.h>
#include <algorithm>

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
	return rPass.addInput(input);
}

ResourceHandle RenderGraphBuilder::addOutput(const ResourceHandle output)
{
	return rPass.addOutput(output);
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

RenderPass& RenderGraph::createRenderPass(Util::String name, SetupFunc& setup, ExecuteFunc&& execute)
{
	// add the pass to the list
	size_t index = renderPasses.size();
	renderPasses.emplace_back(name, index, execute);

	// call the setup function now to initialise this pass
	RenderGraphBuilder builder(this, &renderPasses.back());
	setup(builder);
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

}    // namespace OmegaEngine
