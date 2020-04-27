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

#include "ModelImporter/MaterialInstance.h"
#include "VulkanAPI/Common.h"

#include <memory>

namespace VulkanAPI
{
// forward declerations
class Image;
class ImageView;
class StagingPool;
struct VkContext;
class VkDriver;

/**
 * A simple struct for storing all texture info and ease of passing around
 */
struct TextureContext
{
    vk::Format format;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t mipLevels = 1;
    uint32_t faceCount = 1;
    uint32_t arrayCount = 1;
};

class Texture
{

public:
    using OEFilter = OmegaEngine::MaterialInstance::Sampler::Filter;
    using OEAddressMode = OmegaEngine::MaterialInstance::Sampler::AddressMode;

    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    static vk::Filter toVkFilter(const OEFilter filter);
    static vk::SamplerAddressMode toVkAddressMode(const OEAddressMode mode);

    /**
     * @brief Creates a image and image view for a 2d texture
     * @param format The format of the texture - see **vk::Format**
     * @param width The width of the texture in pixels
     * @param height The height of the texture in pixels
     * @param mipLevels The number of levels this texture contains
     * @param usageFlags The intended usage for this image.
     */
    void create2dTex(
        VkDriver& driver,
        vk::Format format,
        uint32_t width,
        uint32_t height,
        uint8_t mipLevels,
        uint8_t faceCount,
        uint8_t arrayCount,
        vk::ImageUsageFlags usageFlags);

    void createSampler(
        const VkContext& context,
        vk::Filter magFilter,
        vk::Filter minFilter,
        vk::SamplerAddressMode addrModeU,
        vk::SamplerAddressMode addrModeV,
        float maxAntriopsy);

    void createSampler(const VkContext& context, const vk::SamplerCreateInfo& samplerCreateInfo);

    void destroy();

    /**
     * @brief Maps a image in the format specified when the texture as created, to the GPU.
     */
    void map(VkDriver& driver, void* data);

    /**
     * @brief Retuens the image view for this texture
     */
    ImageView* getImageView();

    Image* getImage();

    vk::Sampler& getSampler();

    vk::ImageLayout& getImageLayout();

    /**
     * @brief Returns a struct containing all user relevant info for this texture
     */
    TextureContext& getContext();

private:
    void createCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers);

    void createArrayCopyBuffer(std::vector<vk::BufferImageCopy>& copyBuffers);

private:
    // texture info
    TextureContext texContext;

    // The texture sampler
    vk::Sampler sampler;

    vk::ImageLayout imageLayout = vk::ImageLayout::eUndefined;

    Image* image = nullptr;
    ImageView* imageView = nullptr;
};

} // namespace VulkanAPI
