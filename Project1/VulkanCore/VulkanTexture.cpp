#include "VulkanTexture.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VkMemoryManager.h"
#include "utility/file_log.h"


VulkanTexture::VulkanTexture() :
	width(0),
	height(0),
	size(0),
	mipLevels(1),
	layers(1),
	data(nullptr)
{
}


VulkanTexture::~VulkanTexture()
{
}

// Texture based functions
void VulkanTexture::PrepareImage(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, VulkanEngine *vkEngine, float maxAnisotropy, bool createSampler)
{
	width = w;
	height = h;
	format = f;

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
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usageFlags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; //  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;

	VK_CHECK_RESULT(vkCreateImage(vkEngine->GetDevice(), &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(vkEngine->GetDevice(), image, &mem_req);

	VulkanUtility *p_vkUtility = new VulkanUtility(vkEngine);
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = p_vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(vkEngine->GetDevice(), &alloc_info, nullptr, &texMemory));
	vkBindImageMemory(vkEngine->GetDevice(), image, texMemory, 0);

	// depth image view
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

	VK_CHECK_RESULT(vkCreateImageView(vkEngine->GetDevice(), &createInfo, nullptr, &imageView));

	if (createSampler) {
		CreateTextureSampler(samplerMode, maxAnisotropy, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, vkEngine);
	}

	delete p_vkUtility;
}

void VulkanTexture::PrepareImageArray(const VkFormat f, const VkSamplerAddressMode samplerMode, const VkImageUsageFlags usageFlags, uint32_t w, uint32_t h, uint32_t mips, uint32_t l, VulkanEngine *vkEngine, bool isCube)
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
	image_info.usage = usageFlags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT; //  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	if (isCube) {
		image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
	}

	VK_CHECK_RESULT(vkCreateImage(vkEngine->GetDevice(), &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(vkEngine->GetDevice(), image, &mem_req);

	VulkanUtility *p_vkUtility = new VulkanUtility(vkEngine);
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = p_vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkDeviceMemory texMemory;
	VK_CHECK_RESULT(vkAllocateMemory(vkEngine->GetDevice(), &alloc_info, nullptr, &texMemory));

	vkBindImageMemory(vkEngine->GetDevice(), image, texMemory, 0);

	// depth image view
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

	VK_CHECK_RESULT(vkCreateImageView(vkEngine->GetDevice(), &createInfo, nullptr, &imageView));

	CreateTextureSampler(samplerMode, 1.0, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, vkEngine);
	
	delete p_vkUtility;
}


void VulkanTexture::LoadTexture(std::string filename, const VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VulkanEngine *p_vkEngine, VkMemoryManager *p_vkMemory)
{
	VulkanUtility *p_vkUtility = new VulkanUtility(p_vkEngine);
	
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
	vkMapMemory(p_vkEngine->GetDevice(), stagingMemory, 0, size, 0, &data);
	memcpy(data, tex2d.data(), size);
	vkUnmapMemory(p_vkEngine->GetDevice(), stagingMemory);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = width;
	image_info.extent.height = height;
	image_info.extent.depth = 1;
	image_info.mipLevels = mipLevels;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->GetDevice(), &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->GetDevice(), image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = p_vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkDeviceMemory texMemory;
	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->GetDevice(), &alloc_info, nullptr, &texMemory));

	vkBindImageMemory(p_vkEngine->GetDevice(), image, texMemory, 0);

	p_vkUtility->ImageTransition(VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, mipLevels, 1);

	// copy image
	VkCommandBuffer comm_buff = p_vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

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
	p_vkUtility->SubmitCmdBufferToQueue(comm_buff, p_vkEngine->GetGraphQueue(), p_vkEngine->GetCmdPool());

	p_vkUtility->ImageTransition(VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, mipLevels, 1);

	imageView = p_vkEngine->InitImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);

	CreateTextureSampler(addrMode, maxAnisotropy, color, p_vkEngine);

	vkDestroyBuffer(p_vkEngine->GetDevice(), staging_buff, nullptr);
	vkFreeMemory(p_vkEngine->GetDevice(), stagingMemory, nullptr);

	delete p_vkUtility;
}

void VulkanTexture::LoadTextureArray(std::string filename, const VkSamplerAddressMode addrMode, float maxAnisotropy, const VkBorderColor color, const VkFormat format, const VkCommandPool cmdPool, VulkanEngine *p_vkEngine, VkMemoryManager *p_vkMemory)
{
	VulkanUtility *p_vkUtility = new VulkanUtility(p_vkEngine);
	
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
	vkMapMemory(p_vkEngine->GetDevice(), stagingMemory, 0, size, 0, &data);
	memcpy(data, tex2d.data(), size);
	vkUnmapMemory(p_vkEngine->GetDevice(), stagingMemory);

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

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->GetDevice(), &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->GetDevice(), image, &mem_req);

	// allocate destination buffer
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = p_vkUtility->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	
	VkDeviceMemory texMemory;
	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->GetDevice(), &alloc_info, nullptr, &texMemory));

	vkBindImageMemory(p_vkEngine->GetDevice(), image, texMemory, 0);

	// transition image form undefined to destination transfer
	p_vkUtility->ImageTransition(VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, mipLevels, layers);

	// copy image
	VkCommandBuffer comm_buff = p_vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

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
	p_vkUtility->SubmitCmdBufferToQueue(comm_buff, p_vkEngine->GetGraphQueue(), p_vkEngine->GetCmdPool());

	// now transition image to shader read
	p_vkUtility->ImageTransition(VK_NULL_HANDLE, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, mipLevels, layers);

	// create texture array image view
	imageView = p_vkEngine->InitImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

	CreateTextureSampler(addrMode, maxAnisotropy, color, p_vkEngine);

	vkDestroyBuffer(p_vkEngine->GetDevice(), staging_buff, nullptr);
	vkFreeMemory(p_vkEngine->GetDevice(), stagingMemory, nullptr);

	delete p_vkUtility;
}

void VulkanTexture::LoadCubeMap(std::string filename, const VkFormat format, const VkCommandPool cmdPool, VulkanEngine *p_vkEngine, VkMemoryManager *p_vkMemory)
{
	VulkanUtility *p_vkUtility = new VulkanUtility(p_vkEngine);

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
	vkMapMemory(p_vkEngine->GetDevice(), stagingMemory, 0, size, 0, &data);
	memcpy(data, tex.data(), size);
	vkUnmapMemory(p_vkEngine->GetDevice(), stagingMemory);

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

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->GetDevice(), &image_info, nullptr, &image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->GetDevice(), image, &mem_req);

	// allocate destination buffer
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = p_vkUtility-> FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->GetDevice(), &alloc_info, nullptr, &texMemory));
	vkBindImageMemory(p_vkEngine->GetDevice(), image, texMemory, 0);

	// copy image
	VkCommandBuffer comm_buff = p_vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

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
	p_vkUtility->ImageTransition(comm_buff, image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, mipLevels, 6);

	vkCmdCopyBufferToImage(comm_buff, staging_buff, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());

	// now transition image to shader read
	p_vkUtility->ImageTransition(comm_buff, image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, mipLevels, layers);
	p_vkUtility->SubmitCmdBufferToQueue(comm_buff, p_vkEngine->GetGraphQueue(), p_vkEngine->GetCmdPool());

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
	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->GetDevice(), &createInfo, nullptr, &imageView));

	CreateTextureSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 16.0f, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, p_vkEngine);

	vkDestroyBuffer(p_vkEngine->GetDevice(), staging_buff, nullptr);
	vkFreeMemory(p_vkEngine->GetDevice(), stagingMemory, nullptr);

	delete p_vkUtility;
}

void VulkanTexture::CreateTextureSampler(const VkSamplerAddressMode addressMode, float maxAnisotropy, const VkBorderColor borderColor, VulkanEngine *p_vkEngine)
{
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = VK_FILTER_LINEAR;
	sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = addressMode;
	sampler_info.addressModeV = addressMode;
	sampler_info.addressModeW = addressMode;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = maxAnisotropy;
	sampler_info.borderColor = borderColor;
	sampler_info.unnormalizedCoordinates = VK_FALSE;
	sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_NEVER;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = mipLevels;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;

	VK_CHECK_RESULT(vkCreateSampler(p_vkEngine->GetDevice(), &sampler_info, nullptr, &texSampler));
}



