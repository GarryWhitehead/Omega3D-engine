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

#pragma once

#include "VulkanAPI/Common.h"
#include "VulkanAPI/VkTexture.h"

#include <cstdint>

namespace VulkanAPI
{
// forward decleartions
class Image;
class Texture;
struct VkContext;
class CmdBuffer;

class ImageView
{
public:
    /// no default contructor
    ImageView(VkContext& context);
    ~ImageView();

    /**
     * @brief Returns the aspect flags bsed on the texture format
     */
    static vk::ImageAspectFlags getImageAspect(vk::Format format);

    /**
     * @brief Calculates the view type based on how many faces and whether the texture is an array
     */
    static vk::ImageViewType getTextureType(uint32_t faceCount, uint32_t arrayCount);

    /**
     * @brief Create a new image view based on the specified **Image**
     */
    void create(vk::Device dev, Image& image);

    /**
     * @brief Return the vulkan image view handle
     */
    vk::ImageView& get()
    {
        return imageView;
    }

private:
    vk::Device device;
    vk::ImageView imageView;
};

class Image
{

public:
    Image(
        const VkContext& context,
        const vk::Image& image,
        const vk::Format& format,
        uint32_t width,
        uint32_t height);
    Image(VkContext& context, Texture& tex);
    ~Image();

    /**
     * Returns the interpolation filter based on the format type
     */
    static vk::Filter getFilterType(vk::Format format);

    /**
     * @brief Create a new VkImage instance based on the specified texture  and usage flags
     */
    void create(VmaAllocator& vmaAlloc, vk::ImageUsageFlags usageFlags);

    /**
     *@brief Tansitions the image from one layout to another.
     */
    static void transition(
        Image& image,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout,
        vk::CommandBuffer& cmdBuff,
        uint32_t baseMipMapLevel = UINT32_MAX);

    // =========== static functions ===================
    /**
     * @brief Generates mip maps for the required levels for this image
     */
    void generateMipMap(Image& image, CmdBuffer& cmdBuffer);

    /**
     * @brief Blits the source image to the dst image using the specified
     */
    void blit(Image& srcImage, Image& dstImage, CmdBuffer& cmdBuffer);

    /**
     * @brief Returns the vulkan image handle
     */
    vk::Image& get()
    {
        return image;
    }

    /**
     * @brief Returns the texture context associated with this image
     */
    TextureContext& getContext()
    {
        return tex;
    }

private:
    vk::Device device;
    TextureContext tex;
    vk::Image image;

    // used for memory allocation info such as offsets, etc.
    VmaAllocation imageMem;
};

} // namespace VulkanAPI
