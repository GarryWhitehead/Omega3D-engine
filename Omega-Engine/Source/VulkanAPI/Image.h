#pragma once
#include "VulkanAPI/Common.h"

#include "VulkanAPI/VkTexture.h"

#include <cstdint>

namespace VulkanAPI
{
// forward decleartions
class Image;
class Texture;
class VkContext;
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
	void create(vk::Device dev, Image &image);
    
    /**
    @brief Create a new image view based on the specified **Image**.  Use this function if you need to expilictly specify aspects of the image.
    */
	void create(vk::Device dev, vk::Image &image, vk::Format format, vk::ImageAspectFlags aspect, vk::ImageViewType type);
    
    /**
     * @brief Return the vulkan image view handle
     */
	vk::ImageView &get()
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
	static void transition(Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
	                vk::CommandBuffer &cmdBuff, uint32_t baseMipMapLevel = UINT32_MAX);
    
    // =========== static functions ===================
    // If these should be here or in utils, i'm still undecided!!
    /**
     * @brief Generates mip maps for the required levels for this image
     */
	void generateMipMap(Image& image, CmdBuffer& cmdBuffer);
    
    /**
     * @brief Blits the source image to the dst image using the specified
     */
	void blit(Image& srcImage, Image &dstImage, CmdBuffer& cmdBuffer);
    
    /**
     * @brief Returns the vulkan image handle
     */
	vk::Image &get()
	{
		return image;
	}
    
    /**
     * @brief Returns the texture context associated with this image
     */
	TextureContext &getContext()
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
