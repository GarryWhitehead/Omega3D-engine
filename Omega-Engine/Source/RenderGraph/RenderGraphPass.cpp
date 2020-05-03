/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "RenderGraphPass.h"

#include "RenderGraph/RenderGraph.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/RenderPass.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "utility/Logger.h"

namespace OmegaEngine
{


RenderGraphPass::RenderGraphPass(
    Util::String name, const Type type, RenderGraph& rGraph, const uint32_t index)
    : rGraph(rGraph), name(name), type(type), index(index)
{
}

ResourceHandle RenderGraphPass::addRead(const ResourceHandle input)
{
    // make sure that this handle doesn't already exsist in the list
    // This is just a waste of memory having reduntant resources
    auto iter = std::find(reads.begin(), reads.end(), input);
    if (iter != reads.end())
    {
        return *iter;
    }
    reads.emplace_back(input);
    return input;
}

ResourceHandle RenderGraphPass::addWrite(const ResourceHandle output)
{
    // make sure that this handle doesn't already exsist in the list
    // This is just a waste of memory having reduntant resources
    auto iter = std::find(writes.begin(), writes.end(), output);
    if (iter != writes.end())
    {
        return *iter;
    }
    writes.emplace_back(output);
    return output;
}

void RenderGraphPass::prepare(VulkanAPI::VkDriver& driver)
{
    switch (type)
    {
        case Type::Graphics:
        {
            // a max of six colour attachments allowed per pass
            assert(writes.size() <= 6);

            auto& resources = rGraph.getResources();

            // add the output attachments
            VulkanAPI::VkDriver::RPassKey rpassKey = driver.prepareRPassKey();
            VulkanAPI::VkDriver::FboKey fboKey = driver.prepareFboKey();

            for (size_t i = 0; i < writes.size(); ++i)
            {
                ResourceHandle& handle = writes[i];
                ResourceBase* base = resources[handle];
                if (base->type == ResourceBase::ResourceType::Imported)
                {
                    ImportedResource* tex = static_cast<ImportedResource*>(resources[handle]);

                    // imported targets already have the image view objects defined
                    fboKey.views[i] = tex->imageView.get();
                    rpassKey.colourFormats[i] = tex->format;
                    rpassKey.finalLayout[i] = vk::ImageLayout::ePresentSrcKHR;
                    rpassKey.loadOp = VulkanAPI::LoadClearFlags::Clear;
                    rpassKey.storeOp = VulkanAPI::StoreClearFlags::Store;
                    maxWidth = tex->width;
                    maxHeight = tex->height;
                }
                else if (base->type == ResourceBase::ResourceType::Texture)
                {
                    TextureResource* tex = static_cast<TextureResource*>(resources[handle]);

                    // bake the texture
                    if (tex->width != maxWidth || tex->height != maxHeight)
                    {
                        // not exactly a reason to fatal error maybe, but warn the user
                        // maybe do a blit here instead? The answer is yes, TODO!!!!!
                        tex->width = maxWidth;
                        tex->height = maxHeight;
                        LOGGER_WARN(
                            "There appears to be some discrepancy between this passes resource "
                            "dimensions\n");
                    }
                    VulkanAPI::Texture* vkTexture = tex->bake(driver);
                    fboKey.views[i] = vkTexture->getImageView()->get();

                    if (VulkanAPI::VkUtil::isDepth(tex->format) ||
                        VulkanAPI::VkUtil::isStencil(tex->format))
                    {
                        assert(
                            rpassKey.depth == vk::Format(0) &&
                            "Only one depth reference per renderpass");
                        rpassKey.depth = tex->format;
                    }
                    else
                    {
                        rpassKey.colourFormats[i] = tex->format;
                        rpassKey.finalLayout[i] = vk::ImageLayout::eShaderReadOnlyOptimal;
                        rpassKey.loadOp = tex->loadOp;
                        rpassKey.stencilLoadOp = tex->stencilLoadOp;
                    }
                }
            }

            context.rpass = driver.findOrCreateRenderPass(rpassKey);

            // crete the framebuffer for this pass
            fboKey.renderpass = context.rpass->get();
            fboKey.width = maxWidth;
            fboKey.height = maxHeight;
            context.fbo = driver.findOrCreateFrameBuffer(fboKey);
            break;
        }
        case Type::Compute:
        {
            break;
        }
    }
}

void RenderGraphPass::addExecute(ExecuteFunc&& func)
{
    execFunc = std::move(func);
}

void RenderGraphPass::setClearColour(const OEMaths::colour4& colour)
{
    context.clearCol = colour;
}

void RenderGraphPass::setDepthClear(const float depth)
{
    context.depthClear = depth;
}


} // namespace OmegaEngine
