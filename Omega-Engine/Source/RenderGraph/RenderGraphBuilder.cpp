#include "RenderGraphBuilder.h"

namespace OmegaEngine
{

RenderGraphBuilder::RenderGraphBuilder(RenderGraph* rGraph, RenderGraphPass* rPass)
{
    this->rGraph = rGraph;
    this->rPass = rPass;
}

ResourceHandle RenderGraphBuilder::createTexture(
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const vk::ImageUsageFlagBits usageBits,
    uint32_t levels,
    uint32_t layers)
{
    TextureResource* tex = new TextureResource(width, height, format, levels, layers, usageBits);
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
        LOGGER_ERROR("Unable to find corresponding output attachment whilst trying to add input "
                     "attachment.");
        return UINT64_MAX;
    }

    rPass->addInput(handle);
    return handle;
}

AttachmentHandle
RenderGraphBuilder::addOutputAttachment(Util::String name, const ResourceHandle resource)
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

void RenderGraphBuilder::setClearColour(const OEMaths::colour4& clearCol)
{
    rPass->setClearColour(clearCol);
}

void RenderGraphBuilder::setDepthClear(const float depthClear)
{
    rPass->setDepthClear(depthClear);
}

} // namespace OmegaEngine