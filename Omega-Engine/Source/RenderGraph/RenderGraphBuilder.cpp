#include "RenderGraphBuilder.h"

#include "RenderGraph/RenderGraph.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

RenderGraphBuilder::RenderGraphBuilder(RenderGraph* rGraph, RenderGraphPass* rPass)
{
    this->rGraph = rGraph;
    this->rPass = rPass;
}

ResourceHandle RenderGraphBuilder::createRenderTarget(
    Util::String name,
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const vk::ImageUsageFlags usageBits,
    uint32_t mipLevels,
    uint32_t faceCount)
{
    TextureResource* tex = new TextureResource(name, width, height, format, mipLevels, faceCount, usageBits);
    return rGraph->addResource(reinterpret_cast<ResourceBase*>(tex));
}

ResourceHandle RenderGraphBuilder::importRenderTarget(
    Util::String name,
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const uint8_t samples,
    VulkanAPI::ImageView& view)
{
    return rGraph->importResource(name, view, width, height, format, samples);
}

ResourceHandle RenderGraphBuilder::createBuffer(BufferResource* buffer)
{
    buffer->type = ResourceBase::ResourceType::Buffer;
    return rGraph->addResource(reinterpret_cast<ResourceBase*>(buffer));
}

AttachmentHandle RenderGraphBuilder::addReader(Util::String name)
{
    // link the input with the outputted resource
    AttachmentHandle handle = rGraph->findAttachment(name);
    if (handle == UINT64_MAX)
    {
        LOGGER_ERROR("Unable to find corresponding output attachment whilst trying to add input "
                     "attachment. Reader: %s", name.c_str());
        return UINT64_MAX;
    }

    rPass->addRead(handle);
    return handle;
}

AttachmentHandle
RenderGraphBuilder::addWriter(Util::String name, const ResourceHandle resource)
{
    AttachmentInfo info;
    info.name = name;
    info.resource = resource;

    AttachmentHandle handle = rGraph->addAttachment(info);
    rPass->addWrite(handle);
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
