#include "vulkan_scene.h"
#include "VulkanCore/vulkan_model.h"
#include "VulkanCore/vulkan_terrain.h"
#include "VulkanCore/vulkan_tools.h"


VulkanScene::VulkanScene() : 
	vk_prepared(false)
{	
	p_vulkanModel = new VulkanModel(this);
	p_vulkanTerrain = new VulkanTerrain(this);
}

VulkanScene::~VulkanScene()
{
}

TextureInfo VulkanScene::InitDepthImage()
{
	// required depth image format in order of preference
	std::vector<VkFormat> formats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };

	m_depthImageFormat = p_vulkanModel->FindSupportedFormat(formats, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	TextureInfo depthImage;
	depthImage.width = m_surface.extent.width;
	depthImage.height = m_surface.extent.height;

	VkImageCreateInfo image_info = {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.format = m_depthImageFormat;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.extent.width = depthImage.width;
	image_info.extent.height = depthImage.height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.flags = 0;

	VK_CHECK_RESULT(vkCreateImage(m_device.device, &image_info, nullptr, &depthImage.image));

	VkMemoryRequirements mem_req;
	vkGetImageMemoryRequirements(m_device.device, depthImage.image, &mem_req);

	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_req.size;
	alloc_info.memoryTypeIndex = p_vulkanModel->FindMemoryType(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VK_CHECK_RESULT(vkAllocateMemory(m_device.device, &alloc_info, nullptr, &depthImage.texture_mem));

	vkBindImageMemory(m_device.device, depthImage.image, depthImage.texture_mem, 0);

	depthImage.imageView = InitImageView(depthImage.image, m_depthImageFormat, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_VIEW_TYPE_2D);

	p_vulkanModel->ImageTransition(VK_NULL_HANDLE, depthImage.image, m_depthImageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1, m_cmdPool);

	return depthImage;
}

void VulkanScene::PrepareRenderpass()
{
	VkAttachmentDescription colorAttach = {};
	colorAttach.format = m_surface.format.format;
	colorAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttach.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttach.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttach = {};
	depthAttach.format = m_depthImageFormat;
	depthAttach.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttach.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttach.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttach.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttach.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDependency sPassDepend = {};
	sPassDepend.srcSubpass = VK_SUBPASS_EXTERNAL;
	sPassDepend.dstSubpass = 0;
	sPassDepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	sPassDepend.srcAccessMask = 0;
	sPassDepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDescription sPassDescr = {};
	sPassDescr.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sPassDescr.colorAttachmentCount = 1;
	sPassDescr.pColorAttachments = &colorRef;
	sPassDescr.pDepthStencilAttachment = &depthRef;

	std::vector<VkAttachmentDescription> attach = { colorAttach, depthAttach };
	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.attachmentCount = static_cast<uint32_t>(attach.size());
	createInfo.pAttachments = attach.data();
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &sPassDescr;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &sPassDepend;

	VK_CHECK_RESULT(vkCreateRenderPass(m_device.device, &createInfo, nullptr, &m_renderpass));
}

void VulkanScene::PrepareFrameBuffers()
{
	m_frameBuffer = p_vulkanModel->InitFrameBuffers(m_surface.extent.width, m_surface.extent.height, m_renderpass, m_depthImage.imageView);
}

void VulkanScene::Init()
{
	// start by initialising the "global" renderpass and framebuffer which will be used by most systems
	// image processing modules will use their own offscreen framebuffers though usually the same renderpass
	m_cmdPool = p_vulkanModel->InitCommandPool(m_queue.graphIndex);

	// add depth test image used by the renderpass
	m_depthImage = this->InitDepthImage();

	this->PrepareRenderpass();
	this->PrepareFrameBuffers();

	// now initialise all the vulkan modules
	p_vulkanTerrain->Init();
	p_vulkanModel->InitVulkanModel();

	vk_prepared = true;
}

void VulkanScene::Update(CameraSystem *camera)
{
	p_vulkanTerrain->Update(camera);
	p_vulkanModel->Update(camera);
}

void VulkanScene::RenderFrame()
{

	if (vk_prepared) {

		uint32_t imageIndex = p_vulkanModel->InitRenderFrame();

		// first wait for the offscreen rendering to finish
		VkSubmitInfo submit_info = {};
		VkPipelineStageFlags flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		submit_info.pWaitDstStageMask = &flags;
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.waitSemaphoreCount = 1;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &m_semaphore.image;							// wait for swap chain presentation to finish
		submit_info.pSignalSemaphores = &m_semaphore.render;						// and signal to renderer

		std::vector<VkCommandBuffer> cmdBuffers = { p_vulkanTerrain->m_cmdBuffer[imageIndex], p_vulkanModel->m_cmdBuffer[imageIndex] }; 
		submit_info.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
		submit_info.pCommandBuffers = cmdBuffers.data();

		VK_CHECK_RESULT(vkQueueSubmit(m_queue.graphQueue, 1, &submit_info, VK_NULL_HANDLE));

		//VK_CHECK_RESULT(vkQueueWaitIdle(m_queue.graphQueue));

		p_vulkanModel->SubmitFrame(imageIndex);
	}	

	
}


