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

#include "Resources.h"

#include "RenderGraph/RenderGraph.h"
#include "VulkanAPI/Common.h"
#include "VulkanAPI/Image.h"
#include "VulkanAPI/Utility.h"
#include "VulkanAPI/VkDriver.h"
#include "VulkanAPI/VkTexture.h"


namespace OmegaEngine
{

TextureResource::TextureResource(
    Util::String name,
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const uint8_t mipLevels,
    const uint8_t faceCount,
    const vk::ImageUsageFlags usageBits,
    const VulkanAPI::LoadClearFlags loadOp,
    const VulkanAPI::LoadClearFlags stencilLoadOp)
    : ResourceBase(name, ResourceType::Texture)
    , width(width)
    , height(height)
    , mipLevels(mipLevels)
    , faceCount(faceCount)
    , format(format)
    , imageUsage(usageBits)
    , loadOp(loadOp)
    , stencilLoadOp(stencilLoadOp)
{
}

VulkanAPI::Texture* TextureResource::bake(VulkanAPI::VkDriver& driver)
{
    // TODO: need to add support for arrays too
    return driver.findOrCreateTexture2d(
        name, format, width, height, mipLevels, faceCount, 1, imageUsage);
}


bool TextureResource::isDepthFormat()
{
    if (VulkanAPI::VkUtil::isDepth(format))
    {
        return true;
    }
    return false;
}

bool TextureResource::isColourFormat()
{
    if (!VulkanAPI::VkUtil::isDepth(format) && !VulkanAPI::VkUtil::isStencil(format))
    {
        return true;
    }
    return false;
}

bool TextureResource::isStencilFormat()
{
    if (VulkanAPI::VkUtil::isStencil(format))
    {
        return true;
    }
    return false;
}

ImportedResource::ImportedResource(
    const Util::String& name,
    const uint32_t width,
    const uint32_t height,
    const vk::Format format,
    const uint8_t samples,
    VulkanAPI::ImageView& imageView)
    : ResourceBase(name, ResourceType::Imported)
    , imageView(imageView)
    , width(width)
    , height(height)
    , format(format)
    , samples(samples)
{
}

// =============================================================================


} // namespace OmegaEngine
