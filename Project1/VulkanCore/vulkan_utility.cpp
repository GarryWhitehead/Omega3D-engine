#include "vulkan_utility.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/VulkanEngine.h"
#include "utility/file_log.h"
#include <fstream>
#include "gli.hpp"

VulkanUtility::VulkanUtility()
{
}

VulkanUtility::VulkanUtility(VulkanEngine *engine) :
	p_vkEngine(engine)
{
}


VulkanUtility::~VulkanUtility()
{
}

void VulkanUtility::InitVulkanUtility(VulkanEngine *engine)
{
	p_vkEngine = engine;
}

// =====================================================================================================================================================================================================================================================================================================
// Buffer creation and editing

void VulkanUtility::CreateBuffer(uint32_t size, VkBufferUsageFlags flags, VkMemoryPropertyFlags props, VkBuffer& buffer, VkDeviceMemory& devMemory)
{
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = flags;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK_RESULT(vkCreateBuffer(p_vkEngine->m_device.device, &createInfo, nullptr, &buffer));

	VkMemoryRequirements memoryReq;
	vkGetBufferMemoryRequirements(p_vkEngine->m_device.device, buffer, &memoryReq);

	VkMemoryAllocateInfo memoryInfo = {};
	memoryInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryInfo.allocationSize = memoryReq.size;
	memoryInfo.memoryTypeIndex = FindMemoryType(memoryReq.memoryTypeBits, props);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &memoryInfo, nullptr, &devMemory));

	VK_CHECK_RESULT(vkBindBufferMemory(p_vkEngine->m_device.device, buffer, devMemory, 0));
}

VkWriteDescriptorSet VulkanUtility::InitDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkDescriptorBufferInfo *info)
{
	VkWriteDescriptorSet descrSet = {};
	descrSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descrSet.dstSet = set;
	descrSet.dstBinding = binding;
	descrSet.dstArrayElement = 0;
	descrSet.descriptorCount = 1;
	descrSet.descriptorType = type;
	descrSet.pBufferInfo = info;

	return descrSet;
}

VkWriteDescriptorSet VulkanUtility::InitDescriptorSet(VkDescriptorSet set, uint32_t binding, VkDescriptorType type, VkDescriptorImageInfo *info)
{
	VkWriteDescriptorSet descrSet = {};
	descrSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descrSet.dstSet = set;
	descrSet.dstBinding = binding;
	descrSet.dstArrayElement = 0;
	descrSet.descriptorCount = 1;
	descrSet.descriptorType = type;
	descrSet.pImageInfo = info;

	return descrSet;
}

VkDescriptorImageInfo VulkanUtility::InitImageInfoDescriptor(VkImageLayout layout, VkImageView view, VkSampler sampler)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = layout;
	imageInfo.imageView = view;
	imageInfo.sampler = sampler;

	return imageInfo;
}

VkDescriptorBufferInfo VulkanUtility::InitBufferInfoDescriptor(VkBuffer buffer, int offset, uint32_t range)
{
	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = offset;
	bufferInfo.range = range;

	return bufferInfo;
}

VkDescriptorSetLayoutBinding VulkanUtility::InitLayoutBinding(int binding, VkDescriptorType type, VkShaderStageFlags flags)
{
	VkDescriptorSetLayoutBinding layout = {};
	layout.binding = binding;
	layout.descriptorCount = 1;
	layout.descriptorType = type;
	layout.pImmutableSamplers = nullptr;
	layout.stageFlags = flags;

	return layout;
}

void VulkanUtility::CopyBuffer(VkBuffer dest, VkBuffer src, VkDeviceSize size, VkCommandPool cmdPool)
{
	VkCommandBuffer cmdBuffer;
	cmdBuffer = this->CreateCmdBuffer(VK_PRIMARY, VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

	VkBufferCopy copy;
	copy.size = size;
	copy.dstOffset = 0;
	copy.srcOffset = 0;
	vkCmdCopyBuffer(cmdBuffer, dest, src, 1, &copy);

	this->EndCmdBuffer(cmdBuffer, cmdPool, p_vkEngine->m_queue.graphQueue);
}

uint32_t VulkanUtility::FindMemoryType(uint32_t type, VkMemoryPropertyFlags flags)
{
	VkPhysicalDeviceMemoryProperties memoryProp;
	vkGetPhysicalDeviceMemoryProperties(p_vkEngine->m_device.physDevice, &memoryProp);

	for (int c = 0; c < memoryProp.memoryTypeCount; ++c)
	{
		if ((type & (1 << c)) && (memoryProp.memoryTypes[c].propertyFlags & flags) == flags)
			return c;
	}

	*g_filelog << "Critical Error! Unable to find required memory type.";
	exit(EXIT_FAILURE);
}

// ========================================================================================================================================================================================================================================================================================================
// command buffer tools

std::vector<VkFramebuffer> VulkanUtility::InitFrameBuffers(uint32_t width, uint32_t height, VkRenderPass renderPass, VkImageView imageView = VK_NULL_HANDLE)
{
	int imageCount = p_vkEngine->m_imageView.images.size();
	std::vector<VkFramebuffer> frameBuffers(imageCount);
	std::vector<VkImageView> attach;

	for (int c = 0; c < imageCount; ++c) {

		if (imageView == VK_NULL_HANDLE) {
			attach.resize(1);
			attach = { p_vkEngine->m_imageView.images[c] };
		}
		else {
			attach.resize(2);
			attach = { p_vkEngine->m_imageView.images[c], imageView };
		}

		VkFramebufferCreateInfo frameInfo = {};
		frameInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameInfo.renderPass = renderPass;
		frameInfo.attachmentCount = static_cast<uint32_t>(attach.size());
		frameInfo.pAttachments = attach.data();
		frameInfo.width = width;
		frameInfo.height = height;
		frameInfo.layers = 1;

		VK_CHECK_RESULT(vkCreateFramebuffer(p_vkEngine->m_device.device, &frameInfo, nullptr, &frameBuffers[c]));
	}

	return frameBuffers;
}

VkCommandPool VulkanUtility::InitCommandPool(uint32_t index)
{
	VkCommandPool cmdPool;

	VkCommandPoolCreateInfo commandInfo = {};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandInfo.queueFamilyIndex = index;
	commandInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK_RESULT(vkCreateCommandPool(p_vkEngine->m_device.device, &commandInfo, nullptr, &cmdPool));

	return cmdPool;
}

VkCommandBuffer VulkanUtility::CreateCmdBuffer(bool primary, bool singleUse, VkFramebuffer frameBuffer, VkRenderPass renderPass, VkCommandPool cmdPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = (primary == true) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(p_vkEngine->m_device.device, &allocInfo, &cmdBuffer));

	VkCommandBufferInheritanceInfo inheritInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	if (primary == false) {
		inheritInfo.renderPass = renderPass;
		inheritInfo.framebuffer = frameBuffer;
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = (singleUse == true) ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.flags |= (primary == false) ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT : 0;

	VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &beginInfo));

	return cmdBuffer;
}

void VulkanUtility::SubmitCmdBufferToQueue(VkCommandBuffer cmdBuffer, VkQueue queue)
{
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

	VkFence fence;
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = 0;
	VK_CHECK_RESULT(vkCreateFence(p_vkEngine->m_device.device, &fenceInfo, nullptr, &fence));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence));

	VK_CHECK_RESULT(vkWaitForFences(p_vkEngine->m_device.device, 1, &fence, VK_TRUE, UINT64_MAX));

	vkDestroyFence(p_vkEngine->m_device.device, fence, nullptr);
	vkFreeCommandBuffers(p_vkEngine->m_device.device, p_vkEngine->m_cmdPool, 1, &cmdBuffer);
}

VkCommandBuffer VulkanUtility::CreateTempCmdBuffer(VkCommandPool cmdPool)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(p_vkEngine->m_device.device, &allocInfo, &cmdBuffer));

	return cmdBuffer;
}

void VulkanUtility::EndCmdBuffer(VkCommandBuffer cmdBuffer, VkCommandPool cmdPool, VkQueue queue)
{
	vkEndCommandBuffer(cmdBuffer);

	VkSubmitInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &cmdBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &info, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(queue));

	vkFreeCommandBuffers(p_vkEngine->m_device.device, cmdPool, 1, &cmdBuffer);
}

bool VulkanUtility ::CheckForCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer)
{
	for (auto& buffer : cmdBuffer) {
		if (buffer == VK_NULL_HANDLE) {
			return false;
		}
	}
	return true;
}

void VulkanUtility::DestroyCmdBuffers(std::vector<VkCommandBuffer>& cmdBuffer, VkCommandPool cmdPool)
{
	vkFreeCommandBuffers(p_vkEngine->m_device.device, cmdPool, cmdBuffer.size(), cmdBuffer.data());
}

// =========================================================================================================================================================================================================================================================================================================
// pipeline utilities

VkPipelineViewportStateCreateInfo VulkanUtility::InitViewPortCreateInfo(VkViewport& viewPort, VkRect2D& scissor, float width, float height)
{
	viewPort.x = 0.0f;
	viewPort.y = 0.0f;
	viewPort.width = width;
	viewPort.height = height;
	viewPort.minDepth = 0.0f;
	viewPort.maxDepth = 1.0f;

	scissor.offset = { 0, 0 };
	scissor.extent.width = width;
	scissor.extent.height = height;

	VkPipelineViewportStateCreateInfo viewportInfo = {};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewPort;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	return viewportInfo;
}

VkViewport VulkanUtility::InitViewPort(uint32_t width, uint32_t height, float minDepth, float maxDepth )
{
	VkViewport viewport = {};
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

VkRect2D VulkanUtility::InitScissor(uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
	VkRect2D scissor;
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = x;
	scissor.offset.y = y;
	return scissor;
}

VkPipelineRasterizationStateCreateInfo VulkanUtility::InitRasterzationState(VkPolygonMode polyMode, VkCullModeFlagBits cullMode, VkFrontFace frontFace)
{
	VkPipelineRasterizationStateCreateInfo rasterInfo = {};
	rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterInfo.depthClampEnable = VK_FALSE;
	rasterInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterInfo.polygonMode = polyMode;
	rasterInfo.lineWidth = 1.0f;
	rasterInfo.cullMode = cullMode;
	rasterInfo.frontFace = frontFace;
	rasterInfo.depthBiasEnable = VK_FALSE;

	return rasterInfo;
}

VkPipelineMultisampleStateCreateInfo VulkanUtility::InitMultisampleState(VkSampleCountFlagBits flag)
{
	VkPipelineMultisampleStateCreateInfo multiInfo = {};
	multiInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiInfo.sampleShadingEnable = VK_FALSE;
	multiInfo.rasterizationSamples = flag;
	multiInfo.minSampleShading = 1.0f;
	multiInfo.pSampleMask = nullptr;
	multiInfo.alphaToCoverageEnable = VK_FALSE;
	multiInfo.alphaToOneEnable = VK_FALSE;

	return multiInfo;
}

// =====================================================================================================================================================================================================================================================================================================
// shader utilities and wrapper

VkPipelineShaderStageCreateInfo VulkanUtility::InitShaders(std::string shaderFile, VkShaderStageFlagBits stage)
{
	std::vector<char> shaderData;
	this->LoadFile(shaderFile, shaderData);

	VkShaderModule shader = this->CreateShaderModule(shaderData);

	VkPipelineShaderStageCreateInfo shaderStage = this->CreateShader(shader, stage);

	return shaderStage;
}

void VulkanUtility::LoadFile(std::string filename, std::vector<char>& data)
{
	std::string shaderDir("assets/shaders/");
	std::ifstream file(shaderDir + filename, std::ios_base::ate | std::ios_base::binary);
	if (!file.is_open())
	{
		g_filelog->WriteLog("Critical Error! Unable to open file " + filename);
		exit(EXIT_FAILURE);
	}
	std::ifstream::pos_type filePos = file.tellg();
	g_filelog->WriteLog("size = " + std::to_string(filePos));
	data.resize(filePos);
	file.seekg(0, std::ios_base::beg);
	file.read(data.data(), filePos);
}

VkShaderModule VulkanUtility::CreateShaderModule(std::vector<char>& shader)
{
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = shader.size();
	shaderInfo.pCode = reinterpret_cast<uint32_t*>(shader.data());

	VK_CHECK_RESULT(vkCreateShaderModule(p_vkEngine->m_device.device, &shaderInfo, nullptr, &shaderModule));

	return shaderModule;
}

VkPipelineShaderStageCreateInfo VulkanUtility::CreateShader(VkShaderModule shader, VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage = stage;
	createInfo.module = shader;
	createInfo.pName = "main";

	return createInfo;
}

// ======================================================================================================================================================================================================================================================================================================
// Texture based functions

TextureInfo VulkanUtility::LoadTexture(std::string filename, VkSamplerAddressMode addrMode, VkCompareOp compare, float maxAnisotropy, VkBorderColor color, VkFormat format, VkCommandPool cmdPool)
{
	gli::texture2d tex2d(gli::load(filename.c_str()));
	if (tex2d.size() == 0) {
		*g_filelog << "Critical error! Unable to open texture file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	TextureInfo texture;
	texture.width = static_cast<uint32_t>(tex2d[0].extent().x);
	texture.height = static_cast<uint32_t>(tex2d[0].extent().y);
	texture.size = static_cast<uint32_t>(tex2d.size());
	texture.mipLevels = static_cast<uint32_t>(tex2d.levels());

	VkDeviceMemory stagingMemory;
	VkBuffer staging_buff;
	CreateBuffer(texture.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buff, stagingMemory);

	void *data;
	vkMapMemory(p_vkEngine->m_device.device, stagingMemory, 0, texture.size, 0, &data);
	memcpy(data, tex2d.data(), texture.size);
	vkUnmapMemory(p_vkEngine->m_device.device, stagingMemory);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = texture.width;
	image_info.extent.height = texture.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = texture.mipLevels;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &texture.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, texture.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = this->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &texture.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, texture.image, texture.texture_mem, 0);

	ImageTransition(VK_NULL_HANDLE, texture.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, texture.mipLevels, 1);
	
	// copy image
	VkCommandBuffer comm_buff = CreateCmdBuffer(VK_PRIMARY, VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

	std::vector<VkBufferImageCopy> imageCopyBuffers;
	uint32_t offset = 0;

	for (uint32_t level = 0; level < texture.mipLevels; ++level) {

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

	vkCmdCopyBufferToImage(comm_buff, staging_buff, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());
	this->EndCmdBuffer(comm_buff, cmdPool, p_vkEngine->m_queue.graphQueue);

	ImageTransition(VK_NULL_HANDLE, texture.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, texture.mipLevels, 1);

	texture.imageView = p_vkEngine->InitImageView(texture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D);

	CreateTextureSampler(texture, addrMode, 16, compare, color);

	vkDestroyBuffer(p_vkEngine->m_device.device, staging_buff, nullptr);
	vkFreeMemory(p_vkEngine->m_device.device, stagingMemory, nullptr);

	return texture;
}

TextureInfo VulkanUtility::LoadTextureArray(std::string filename, VkSamplerAddressMode addrMode, VkCompareOp compare, float maxAnisotropy, VkBorderColor color, VkFormat format, VkCommandPool cmdPool)
{
	gli::texture2d_array tex2d(gli::load(filename.c_str()));
	if (tex2d.size() == 0) {
		*g_filelog << "Critical error! Unable to open texture file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	TextureInfo texture;
	texture.width = static_cast<uint32_t>(tex2d[0].extent().x);
	texture.height = static_cast<uint32_t>(tex2d[0].extent().y);
	texture.size = static_cast<uint32_t>(tex2d.size());
	texture.mipLevels = static_cast<uint32_t>(tex2d.levels());
	texture.layers = static_cast<uint32_t>(tex2d.layers());
	texture.data = tex2d.data();

	VkDeviceMemory stagingMemory;
	VkBuffer staging_buff;
	CreateBuffer(texture.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buff, stagingMemory);

	// copy image to staging buffer
	void *data;
	vkMapMemory(p_vkEngine->m_device.device, stagingMemory, 0, texture.size, 0, &data);
	memcpy(data, texture.data, texture.size);
	vkUnmapMemory(p_vkEngine->m_device.device, stagingMemory);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = texture.width;
	image_info.extent.height = texture.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = texture.mipLevels;
	image_info.arrayLayers = texture.layers;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &texture.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, texture.image, &mem_req);

	// allocate destination buffer
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = this->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &texture.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, texture.image, texture.texture_mem, 0);

	// transition image form undefined to destination transfer
	ImageTransition(VK_NULL_HANDLE, texture.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, texture.mipLevels, texture.layers);
	
	// copy image
	VkCommandBuffer comm_buff = CreateCmdBuffer(VK_PRIMARY, VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

	std::vector<VkBufferImageCopy> imageCopyBuffers;
	uint32_t offset = 0;

	for (uint32_t layer = 0; layer < texture.layers; ++layer) {

		for (uint32_t level = 0; level < texture.mipLevels; ++level) {

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

	vkCmdCopyBufferToImage(comm_buff, staging_buff, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());
	this->EndCmdBuffer(comm_buff, cmdPool, p_vkEngine->m_queue.graphQueue);
	
	// now transition image to shader read
	ImageTransition(VK_NULL_HANDLE, texture.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, texture.mipLevels, texture.layers);

	// create texture array image view
	texture.imageView = p_vkEngine->InitImageView(texture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

	CreateTextureSampler(texture, addrMode, 16, compare, color);

	vkDestroyBuffer(p_vkEngine->m_device.device, staging_buff, nullptr);
	vkFreeMemory(p_vkEngine->m_device.device, stagingMemory, nullptr);

	return texture;
}

TextureInfo VulkanUtility::LoadCubeMap(std::string filename, VkFormat format, VkCommandPool cmdPool)
{
	gli::texture_cube tex(gli::load(filename.c_str()));
	if (tex.size() == 0) {
		*g_filelog << "Critical error! Unable to open texture file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}

	TextureInfo texture;
	texture.width = static_cast<uint32_t>(tex.extent().x);
	texture.height = static_cast<uint32_t>(tex.extent().y);
	texture.size = static_cast<uint32_t>(tex.size());
	texture.mipLevels = static_cast<uint32_t>(tex.levels());
	texture.layers = 6;
	texture.data = tex.data();

	VkDeviceMemory stagingMemory;
	VkBuffer staging_buff;
	CreateBuffer(texture.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, staging_buff, stagingMemory);

	// copy image to staging buffer
	void *data;
	vkMapMemory(p_vkEngine->m_device.device, stagingMemory, 0, texture.size, 0, &data);
	memcpy(data, texture.data, texture.size);
	vkUnmapMemory(p_vkEngine->m_device.device, stagingMemory);

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = format;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = texture.width;
	image_info.extent.height = texture.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = texture.mipLevels;
	image_info.arrayLayers = 6;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VK_CHECK_RESULT(vkCreateImage(p_vkEngine->m_device.device, &image_info, nullptr, &texture.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(p_vkEngine->m_device.device, texture.image, &mem_req);

	// allocate destination buffer
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = this->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(p_vkEngine->m_device.device, &alloc_info, nullptr, &texture.texture_mem));

	vkBindImageMemory(p_vkEngine->m_device.device, texture.image, texture.texture_mem, 0);

	// copy image
	VkCommandBuffer comm_buff = CreateCmdBuffer(VK_PRIMARY, VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);

	std::vector<VkBufferImageCopy> imageCopyBuffers;
	uint32_t offset = 0;

	for (uint32_t layer = 0; layer < 6; ++layer) {

		for (uint32_t level = 0; level < texture.mipLevels; ++level) {

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
	ImageTransition(comm_buff, texture.image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmdPool, texture.mipLevels, 6);

	vkCmdCopyBufferToImage(comm_buff, staging_buff, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(imageCopyBuffers.size()), imageCopyBuffers.data());

	// now transition image to shader read
	ImageTransition(comm_buff, texture.image, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmdPool, texture.mipLevels, texture.layers);
	this->EndCmdBuffer(comm_buff, cmdPool, p_vkEngine->m_queue.graphQueue);

	// create texture array image view
	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = texture.image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	createInfo.format = format;
	createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 6;
	createInfo.subresourceRange.levelCount = texture.mipLevels;
	createInfo.subresourceRange.baseArrayLayer = 0;
	VK_CHECK_RESULT(vkCreateImageView(p_vkEngine->m_device.device, &createInfo, nullptr, &texture.imageView));

	CreateTextureSampler(texture, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, 16.0f, VK_COMPARE_OP_NEVER, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE);

	vkDestroyBuffer(p_vkEngine->m_device.device, staging_buff, nullptr);
	vkFreeMemory(p_vkEngine->m_device.device, stagingMemory, nullptr);

	return texture;
}

void VulkanUtility::ImageTransition(const VkCommandBuffer cmdBuff, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, VkCommandPool cmdPool, uint32_t mipLevels, uint32_t layers)
{
	VkCommandBuffer comm_buff;
	if (cmdBuff == VK_NULL_HANDLE) {
		comm_buff = this->CreateCmdBuffer(VK_PRIMARY, VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool);
	}
	else {
		comm_buff = cmdBuff;
	}

	VkImageMemoryBarrier mem_barr = {};
	mem_barr.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	mem_barr.oldLayout = old_layout;
	mem_barr.newLayout = new_layout;
	mem_barr.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	mem_barr.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	mem_barr.image = image;

	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {

			mem_barr.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		else {
			mem_barr.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		}
	}
	else {
		mem_barr.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	mem_barr.subresourceRange.baseArrayLayer = 0;
	mem_barr.subresourceRange.layerCount = layers;
	mem_barr.subresourceRange.baseMipLevel = 0;
	mem_barr.subresourceRange.levelCount = mipLevels;

	VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		mem_barr.srcAccessMask = 0;
		mem_barr.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		mem_barr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		mem_barr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		mem_barr.srcAccessMask = 0;
		mem_barr.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		mem_barr.srcAccessMask = 0;
		mem_barr.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		mem_barr.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		mem_barr.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		mem_barr.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		mem_barr.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	else {
		g_filelog->WriteLog("Invalid parameters for image transition.");
		exit(EXIT_FAILURE);
	}

	vkCmdPipelineBarrier(comm_buff, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &mem_barr);

	if (cmdBuff == VK_NULL_HANDLE) {
		this->EndCmdBuffer(comm_buff, cmdPool, p_vkEngine->m_queue.graphQueue);
	}
}

void VulkanUtility::CreateTextureSampler(TextureInfo& texture, VkSamplerAddressMode addressMode, float maxAnisotropy, VkCompareOp compareOp, VkBorderColor borderColor)
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
	sampler_info.compareOp = compareOp;
	sampler_info.minLod = 0.0f;
	sampler_info.maxLod = texture.mipLevels;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.mipLodBias = 0.0f;

	VK_CHECK_RESULT(vkCreateSampler(p_vkEngine->m_device.device, &sampler_info, nullptr, &texture.m_tex_sampler));
}

VkFormat VulkanUtility::FindSupportedFormat(const std::vector<VkFormat>& requiredFormats, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (auto format : requiredFormats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(p_vkEngine->m_device.physDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && features == (props.linearTilingFeatures & features)) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && features == (props.optimalTilingFeatures & features)) {
			return format;
		}
		else {
			g_filelog->WriteLog("Critical error! No required format found");
			exit(EXIT_FAILURE);
		}
	}
}

// =========================================================================================================================================================================================================================================================================================================
// Render functions

uint32_t VulkanUtility::InitRenderFrame()
{
	uint32_t image_index;
	vkAcquireNextImageKHR(p_vkEngine->m_device.device, p_vkEngine->m_swapchain.swapChain, std::numeric_limits<uint64_t>::max(), p_vkEngine->m_semaphore.image, VK_NULL_HANDLE, &image_index);

	return image_index;
}

void VulkanUtility::SubmitFrame(uint32_t imageIndex)
{
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &p_vkEngine->m_semaphore.render;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &p_vkEngine->m_swapchain.swapChain;
	present_info.pImageIndices = &imageIndex;

	VK_CHECK_RESULT(vkQueuePresentKHR(p_vkEngine->m_queue.presentQueue, &present_info));

	VK_CHECK_RESULT(vkQueueWaitIdle(p_vkEngine->m_queue.presentQueue));
}