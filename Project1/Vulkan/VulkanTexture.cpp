#include "VulkanTexture.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VkMemoryManager.h"
#include "utility/file_log.h"


VulkanTexture::VulkanTexture(VkPhysicalDevice p_dev, VkDevice dev) :
	width(0),
	height(0),
	size(0),
	mipLevels(1),
	layers(1),
	data(nullptr),
	texSampler(VK_NULL_HANDLE),
	physDevice(p_dev),
	device(dev)
{
}


VulkanTexture::~VulkanTexture()
{
	Destroy();
}

// Texture based functions
void VulkanTexture::PrepareImage(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, float maxAnisotropy, bool createSampler, uint32_t mips, VkFilter filter)
{
	width = w;
	height = h;
	format = f;
	mipLevels = mips;

	VkImageAspectFlags aspectFlags = 0;

	if (usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	assert(aspectFlags > 0);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mipLevels;			// used for creating dynamic mipmaps 
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usageFlags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; 
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK_RESULT(vkCreateImage(device, &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(device, image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice);

	VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, nullptr, &texMemory));
	vkBindImageMemory(device, image, texMemory, 0);

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(device, &createInfo, nullptr, &imageView));

	if (createSampler) {
		CreateTextureSampler(samplerMode, maxAnisotropy, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, filter);
	}
}

void VulkanTexture::PrepareImageArray(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, uint32_t mips, uint32_t l, bool isCube)
{
	width = w;
	height = h;
	format = f;
	mipLevels = mips;
	layers = l;

	VkImageAspectFlags aspectFlags = 0;
	
	if (usageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else {
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	assert(aspectFlags > 0);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mipLevels;
	image_info.arrayLayers = layers;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usageFlags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; 
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	if (isCube) {
		image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VK_CHECK_RESULT(vkCreateImage(device, &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(device, image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice);

	VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, nullptr, &texMemory));

	vkBindImageMemory(device, image, texMemory, 0);

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = (isCube) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = layers;
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VK_CHECK_RESULT(vkCreateImageView(device, &createInfo, nullptr, &imageView));

	CreateTextureSampler(samplerMode, 1.0, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FILTER_LINEAR);
}

void VulkanTexture::UploadDataToImage(void* tex_data, uint32_t size, VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory)
{
	assert(tex_data != nullptr);

	// create a temp buffer to upload the image data
	VkBuffer staging_buff;
	VkDeviceMemory staging_mem;
	p_vkMemory->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_mem, staging_buff);

	void *data;
	vkMapMemory(device, staging_mem, 0, size, 0, &data);
	memcpy(data, tex_data, size);
	vkUnmapMemory(device, staging_mem);

	VkCommandBuffer comm_buff = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

	VkBufferImageCopy image_copy = {};
	image_copy.imageExtent.width = width;
	image_copy.imageExtent.height = height;
	image_copy.imageExtent.depth = 1;
	image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	image_copy.imageSubresource.layerCount = 1;

	// transition image to transfer state
	VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, device);

	// copy data to image buffer
	vkCmdCopyBufferToImage(comm_buff, staging_buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
	VulkanUtility::SubmitCmdBufferToQueue(comm_buff, graphQueue, cmdPool, device);

	// transition image to shader read state
	VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, device);

	//clean up
	vkDestroyBuffer(device, staging_buff, nullptr);
	vkFreeMemory(device, staging_mem, nullptr);
}

void VulkanTexture::GenerateMipChain(uint32_t mipLevels, VkCommandPool cmdPool, VkQueue graphQueue)
{
	assert(image != VK_NULL_HANDLE);

	// create command buffer for the blitting
	VkCommandBuffer cmdBuffer = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

	// transition the base image to source
	VulkanUtility::ImageTransition(graphQueue, cmdBuffer, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmdPool, device);

	for (int32_t c = 1; c < mipLevels; ++c) {

		VkImageBlit blit = {};
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = c - 1;		// previous mipmap generated used as the base for the blit
		blit.srcOffsets[1].x = static_cast<int32_t>(width >> (c - 1));
		blit.srcOffsets[1].y = static_cast<int32_t>(height >> (c - 1));
		blit.srcOffsets[1].z = 1;

		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.layerCount = 1;
		blit.dstSubresource.mipLevel = c;
		blit.dstOffsets[1].x = static_cast<int32_t>(width >> c);
		blit.dstOffsets[1].y = static_cast<int32_t>(height >> c);
		blit.dstOffsets[1].z = 1;

		// prepare base image for transfer
		VulkanUtility::ImageTransition(graphQueue, cmdBuffer, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, device, 1, 1, c);

		// blit image from previous to next image - image is downscaled with linear filtering
		vkCmdBlitImage(cmdBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		// prepare image for next blit
		VulkanUtility::ImageTransition(graphQueue, cmdBuffer, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmdPool, device, 1, 1, c);
	}

	// finish by preparing image for shader read
	VulkanUtility::ImageTransition(graphQueue, cmdBuffer, image, format, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, device, mipLevels, 1);

	// flush cmd buffer
	VulkanUtility::SubmitCmdBufferToQueue(cmdBuffer, graphQueue, cmdPool, device);
}

void VulkanTexture::LoadTexture(std::string filename, const VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory)
{	
	gli::texture2d tex2d(gli::load(filename.c_str()));
	if (tex2d.size() == 0) {
		*g_filelog << "Critical error! Unable to open texture file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	width = static_cast<uint32_t>(tex2d[0].extent().x);
	height = static_cast<uint32_t>(tex2d[0].extent().y);
	size = static_cast<uint32_t>(tex2d.size());
	mipLevels = static_cast<uint32_t>(tex2d.levels());

	VkDeviceMemory stagingMemory;
	VkBuffer staging_buff;
	p_vkMemory->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory, staging_buff);

	void *data;
	vkMapMemory(device, stagingMemory, 0, size, 0, &data);
	memcpy(data, tex2d.data(), size);
	vkUnmapMemory(device, stagingMemory);

	

	VK_CHECK_RESULT(vkCreateImage(device, &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(device, image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice);

	VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, nullptr, &texMemory));

	vkBindImageMemory(device, image, texMemory, 0);

	VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, device, mipLevels, 1);

	// copy image
	VkCommandBuffer comm_buff = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

	std::vector<VkBufferImageCopy> imageCopyBuffers;
	uint32_t offset = 0;

	for (uint32_t level = 0; level < mipLevels; ++level) {

		VkBufferImageCopy image_copy = {};
		image_copy.imageExtent.width = static_cast<uint32_t>(tex2d[level].extent().x);
		image_copy.imageExtent.height = static_cast<uint32_t>(tex2d[level].extent().y);
		image_copy.imageExtent.depth = 1;
		image_copy.bufferOffset = 0;
		image_copy.bufferRowLength = 0;
		image_copy.bufferImageHeight = 0;
		image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_copy.imageSubresource.mipLevel = level;
		image_copy.imageSubresource.layerCount = 1;
		image_copy.imageSubresource.baseArrayLayer = 0;
		image_copy.imageOffset = { 0,0,0 };
		imageCopyBuffers.emplace_back(image_copy);

		offset += static_cast<uint32_t>(tex2d[level].size());
	}

	vkCmdCopyBufferToImage(comm_buff, staging_buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());
	VulkanUtility::SubmitCmdBufferToQueue(comm_buff, graphQueue, cmdPool, device);

	VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, device, mipLevels, 1);

	imageView = VulkanUtility::InitImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, device);

	CreateTextureSampler(addrMode, maxAnisotropy, color, VK_FILTER_LINEAR);

	vkDestroyBuffer(device, staging_buff, nullptr);
	vkFreeMemory(device, stagingMemory, nullptr);
}

void VulkanTexture::LoadTextureArray(std::string filename, const VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory)
{	
	gli::texture2d_array tex2d(gli::load(filename.c_str()));
	if (tex2d.size() == 0) {
		*g_filelog << "Critical error! Unable to open texture file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	width = static_cast<uint32_t>(tex2d[0].extent().x);
	height = static_cast<uint32_t>(tex2d[0].extent().y);
	size = static_cast<uint32_t>(tex2d.size());
	mipLevels = static_cast<uint32_t>(tex2d.levels());
	layers = static_cast<uint32_t>(tex2d.layers());

	VkDeviceMemory stagingMemory;
	VkBuffer staging_buff;
	p_vkMemory->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory, staging_buff);

	// copy image to staging buffer
	void *data;
	vkMapMemory(device, stagingMemory, 0, size, 0, &data);
	memcpy(data, tex2d.data(), size);
	vkUnmapMemory(device, stagingMemory);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mipLevels;
	image_info.arrayLayers = layers;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(device, &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(device, image, &mem_req);

	// allocate destination buffer
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice);
	
	VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, nullptr, &texMemory));

	vkBindImageMemory(device, image, texMemory, 0);

	// transition image form undefined to destination transfer
	VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, device, mipLevels, layers);

	// copy image
	VkCommandBuffer comm_buff = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

	std::vector<VkBufferImageCopy> imageCopyBuffers;
	uint32_t offset = 0;

	for (uint32_t layer = 0; layer < layers; ++layer) {

		for (uint32_t level = 0; level < mipLevels; ++level) {

			VkBufferImageCopy image_copy = {};
			image_copy.imageExtent.width = static_cast<uint32_t>(tex2d[layer][level].extent().x);
			image_copy.imageExtent.height = static_cast<uint32_t>(tex2d[layer][level].extent().y);
			image_copy.imageExtent.depth = 1;
			image_copy.bufferOffset = offset;
			image_copy.bufferRowLength = 0;
			image_copy.bufferImageHeight = 0;
			image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_copy.imageSubresource.mipLevel = level;
			image_copy.imageSubresource.layerCount = 1;
			image_copy.imageSubresource.baseArrayLayer = layer;
			image_copy.imageOffset = { 0,0,0 };
			imageCopyBuffers.emplace_back(image_copy);

			offset += static_cast<uint32_t>(tex2d[layer][level].size());
		}
	}

	vkCmdCopyBufferToImage(comm_buff, staging_buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());
	VulkanUtility::SubmitCmdBufferToQueue(comm_buff, graphQueue, cmdPool, device);

	// now transition image to shader read
	VulkanUtility::ImageTransition(graphQueue, VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, device, mipLevels, layers);

	// create texture array image view
	imageView = VulkanUtility::InitImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY, device);

	CreateTextureSampler(addrMode, maxAnisotropy, color, VK_FILTER_LINEAR);

	vkDestroyBuffer(device, staging_buff, nullptr);
	vkFreeMemory(device, stagingMemory, nullptr);

}

void VulkanTexture::LoadCubeMap(std::string filename, const VkFormat format, const VkCommandPool cmdPool, VkQueue graphQueue, VkMemoryManager *p_vkMemory)
{
	gli::texture_cube tex(gli::load(filename.c_str()));
	if (tex.size() == 0) {
		*g_filelog << "Critical error! Unable to open texture file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	width = static_cast<uint32_t>(tex.extent().x);
	height = static_cast<uint32_t>(tex.extent().y);
	size = static_cast<uint32_t>(tex.size());
	mipLevels = static_cast<uint32_t>(tex.levels());
	layers = 6;

	VkDeviceMemory stagingMemory;
	VkBuffer staging_buff;
	p_vkMemory->CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory, staging_buff);

	// copy image to staging buffer
	void *data;
	vkMapMemory(device, stagingMemory, 0, size, 0, &data);
	memcpy(data, tex.data(), size);
	vkUnmapMemory(device, stagingMemory);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mipLevels;
	image_info.arrayLayers = 6;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VK_CHECK_RESULT(vkCreateImage(device, &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(device, image, &mem_req);

	// allocate destination buffer
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = VulkanUtility::FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physDevice);

	VK_CHECK_RESULT(vkAllocateMemory(device, &alloc_info, nullptr, &texMemory));
	vkBindImageMemory(device, image, texMemory, 0);

	// copy image
	VkCommandBuffer comm_buff = VulkanUtility::CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

	std::vector<VkBufferImageCopy> imageCopyBuffers;
	uint32_t offset = 0;

	for (uint32_t layer = 0; layer < 6; ++layer) {

		for (uint32_t level = 0; level < mipLevels; ++level) {

			VkBufferImageCopy image_copy = {};
			image_copy.imageExtent.width = static_cast<uint32_t>(tex[layer][level].extent().x);
			image_copy.imageExtent.height = static_cast<uint32_t>(tex[layer][level].extent().y);
			image_copy.imageExtent.depth = 1;
			image_copy.bufferOffset = offset;
			image_copy.bufferRowLength = 0;
			image_copy.bufferImageHeight = 0;
			image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			image_copy.imageSubresource.mipLevel = level;
			image_copy.imageSubresource.layerCount = 1;
			image_copy.imageSubresource.baseArrayLayer = layer;
			image_copy.imageOffset = { 0,0,0 };
			imageCopyBuffers.emplace_back(image_copy);

			offset += static_cast<uint32_t>(tex[layer][level].size());
		}
	}

	// transition image form undefined to destination transfer
	VulkanUtility::ImageTransition(graphQueue, comm_buff, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, device, mipLevels, 6);

	vkCmdCopyBufferToImage(comm_buff, staging_buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());

	// now transition image to shader read
	VulkanUtility::ImageTransition(graphQueue, comm_buff, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, device, mipLevels, layers);
	VulkanUtility::SubmitCmdBufferToQueue(comm_buff, graphQueue, cmdPool, device);

	// create texture array image view
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	createInfo.format = format;
	createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 6;
	createInfo.subresourceRange.levelCount = mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	VK_CHECK_RESULT(vkCreateImageView(device, &createInfo, nullptr, &imageView));

	CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FILTER_LINEAR);

	vkDestroyBuffer(device, staging_buff, nullptr);
	vkFreeMemory(device, stagingMemory, nullptr);
}

void VulkanTexture::CreateTextureSampler(const VkSamplerAddressMode addressMode, float maxAnisotropy, const VkBorderColor borderColor, VkFilter filter)
{
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = filter;
	sampler_info.minFilter = filter;
	sampler_info.addressModeU = addressMode;
	sampler_info.addressModeV = addressMode;
	sampler_info.addressModeW = addressMode;
	sampler_info.anisotropyEnable = maxAnisotropy == 0.0f ? VK_FALSE : VK_TRUE;
	sampler_info.maxAnisotropy = maxAnisotropy;
	sampler_info.borderColor = borderColor;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_NEVER;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = (float)mipLevels;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;

	VK_CHECK_RESULT(vkCreateSampler(device, &sampler_info, nullptr, &texSampler));
}

void VulkanTexture::Destroy()
{
	
	if (texSampler != VK_NULL_HANDLE) {

		vkDestroySampler(device, texSampler, nullptr);
	}
	vkDestroyImageView(device, imageView, nullptr);
	vkDestroyImage(device, image, nullptr);
	vkFreeMemory(device, texMemory, nullptr);
}


