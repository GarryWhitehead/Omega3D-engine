#include "vulkan_utility.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/VulkanEngine.h"
#include "utility/file_log.h"
#include <fstream>
#include "gli.hpp"

VulkanUtility::VulkanUtility()
{
}


VulkanUtility::~VulkanUtility()
{
}

// =====================================================================================================================================================================================================================================================================================================
//  buffer tools 

void VulkanUtility::CopyBuffer(const VkBuffer src, const VkBuffer dest, const VkDeviceSize size, const VkCommandPool cmdPool, VkQueue graphQueue, VkDevice device, const uint32_t srcOffset, const uint32_t dstOffset)
{
	VkCommandBuffer cmdBuffer = CreateCmdBuffer(VK_PRIMARY, VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);

	VkBufferCopy copy = {};
	copy.size = size;
	copy.dstOffset = dstOffset;
	copy.srcOffset = srcOffset;
	vkCmdCopyBuffer(cmdBuffer, src, dest, 1, &copy);

	SubmitCmdBufferToQueue(cmdBuffer, graphQueue, cmdPool, device);
}

uint32_t VulkanUtility::FindMemoryType(const uint32_t type, const VkMemoryPropertyFlags flags, VkPhysicalDevice physDevice)
{
	VkPhysicalDeviceMemoryProperties memoryProp;
	vkGetPhysicalDeviceMemoryProperties(physDevice, &memoryProp);

	for (uint32_t c = 0; c < memoryProp.memoryTypeCount; ++c)
	{
		if ((type & (1 << c)) && (memoryProp.memoryTypes[c].propertyFlags & flags) == flags)
			return c;
	}

	*g_filelog << "Critical Error! Unable to find required memory type.";
	exit(EXIT_FAILURE);
}

// ========================================================================================================================================================================================================================================================================================================
// command buffer tools

VkCommandPool VulkanUtility::InitCommandPool(const uint32_t index, VkDevice device)
{
	VkCommandPool cmdPool;

	VkCommandPoolCreateInfo commandInfo = {};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandInfo.queueFamilyIndex = index;
	commandInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VK_CHECK_RESULT(vkCreateCommandPool(device, &commandInfo, nullptr, &cmdPool));

	return cmdPool;
}

VkCommandBuffer VulkanUtility::CreateCmdBuffer(bool primary, bool singleUse, const VkFramebuffer frameBuffer, const VkRenderPass renderPass, const VkCommandPool cmdPool, VkDevice device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = (primary == true) ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocInfo, &cmdBuffer));

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

void VulkanUtility::SubmitCmdBufferToQueue(const VkCommandBuffer cmdBuffer, const VkQueue queue, const VkCommandPool cmdPool, VkDevice device)
{
	VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(queue));

	vkFreeCommandBuffers(device, cmdPool, 1, &cmdBuffer);
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


// =========================================================================================================================================================================================================================================================================================================
// pipeline utilities

VkPipelineViewportStateCreateInfo VulkanUtility::InitViewPortCreateInfo(VkViewport& viewPort, VkRect2D& scissor, const uint32_t width, const uint32_t height)
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

VkViewport VulkanUtility::InitViewPort(const uint32_t width, const uint32_t height, const float minDepth, const float maxDepth )
{
	VkViewport viewport = {};
	viewport.width = width;
	viewport.height = height;
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

VkRect2D VulkanUtility::InitScissor(const uint32_t width, const uint32_t height, const uint32_t x, const uint32_t y)
{
	VkRect2D scissor;
	scissor.extent.width = width;
	scissor.extent.height = height;
	scissor.offset.x = x;
	scissor.offset.y = y;
	return scissor;
}

VkPipelineRasterizationStateCreateInfo VulkanUtility::InitRasterzationState(const VkPolygonMode polyMode, const VkCullModeFlagBits cullMode, const VkFrontFace frontFace)
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

VkPipelineMultisampleStateCreateInfo VulkanUtility::InitMultisampleState(const VkSampleCountFlagBits flag)
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

VkPipelineShaderStageCreateInfo VulkanUtility::InitShaders(std::string shaderFile, const VkShaderStageFlagBits stage, VkDevice device)
{
	std::vector<char> shaderData;
	LoadFile(shaderFile, shaderData);

	VkShaderModule shader = CreateShaderModule(shaderData, device);

	VkPipelineShaderStageCreateInfo shaderStage = CreateShader(shader, stage);

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
	data.resize(filePos);
	file.seekg(0, std::ios_base::beg);
	file.read(data.data(), filePos);
}

VkShaderModule VulkanUtility::CreateShaderModule(std::vector<char>& shader, VkDevice device)
{
	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = shader.size();
	shaderInfo.pCode = reinterpret_cast<uint32_t*>(shader.data());

	VK_CHECK_RESULT(vkCreateShaderModule(device, &shaderInfo, nullptr, &shaderModule));

	return shaderModule;
}

VkPipelineShaderStageCreateInfo VulkanUtility::CreateShader(const VkShaderModule shader, const VkShaderStageFlagBits stage)
{
	VkPipelineShaderStageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.stage = stage;
	createInfo.module = shader;
	createInfo.pName = "main";

	return createInfo;
}

// ===========
void VulkanUtility::ImageTransition(const VkQueue graphQueue, const VkCommandBuffer cmdBuff, const VkImage image, const VkFormat format, const VkImageLayout old_layout, const VkImageLayout new_layout, const VkCommandPool cmdPool, VkDevice device, uint32_t levelCount, uint32_t layers, uint32_t mipLevel)
{

	VkCommandBuffer comm_buff;
	if (cmdBuff == VK_NULL_HANDLE) {
		comm_buff = CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_SINGLE_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, cmdPool, device);
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
	mem_barr.subresourceRange.baseMipLevel = mipLevel;
	mem_barr.subresourceRange.levelCount = levelCount;

	VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		mem_barr.srcAccessMask = 0;
		mem_barr.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		mem_barr.srcAccessMask = 0;
		mem_barr.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		mem_barr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		mem_barr.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	}

	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		mem_barr.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
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

	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
		mem_barr.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
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

		SubmitCmdBufferToQueue(comm_buff, graphQueue, cmdPool, device);
	}
}

VkFormat VulkanUtility::FindSupportedFormat(const std::vector<VkFormat>& requiredFormats, const VkImageTiling tiling, const VkFormatFeatureFlags features, VkPhysicalDevice physDevice)
{
	for (auto format : requiredFormats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physDevice, format, &props);

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

VkImageView VulkanUtility::InitImageView(VkImage image, VkFormat format, VkImageAspectFlagBits imageAspect, VkImageViewType type, VkDevice device)
{

	VkImageViewCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = type;
	createInfo.format = format;
	createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	createInfo.subresourceRange.aspectMask = imageAspect;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.layerCount = 1;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;

	VkImageView imageView;
	VK_CHECK_RESULT(vkCreateImageView(device, &createInfo, nullptr, &imageView));

	return imageView;
}

// =========================================================================================================================================================================================================================================================================================================
// Render functions

uint32_t VulkanUtility::InitRenderFrame(VkDevice device, VkSwapchainKHR swapChain, VkSemaphore imageSem)
{
	uint32_t image_index;
	vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageSem, VK_NULL_HANDLE, &image_index);

	return image_index;
}

void VulkanUtility::SubmitFrame(uint32_t imageIndex, VkSwapchainKHR swapChain, VkSemaphore renderSem, VkQueue presentQueue)
{
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &renderSem;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapChain;
	present_info.pImageIndices = &imageIndex;

	VK_CHECK_RESULT(vkQueuePresentKHR(presentQueue, &present_info));

	VK_CHECK_RESULT(vkQueueWaitIdle(presentQueue));
}

// ==============================================================================================================================================================================================================================================================================
// barrier tools
VkSemaphore VulkanUtility::CreateSemaphore(VkDevice device)
{
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo semaphore_info = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphore_info, nullptr, &semaphore));

	return semaphore;
}

VkFence VulkanUtility::CreateFence(VkFenceCreateFlags flags, VkDevice device)
{
	VkFence fence;
	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.flags = flags;

	VK_CHECK_RESULT(vkCreateFence(device, &createInfo, nullptr, &fence));
	return fence;
}