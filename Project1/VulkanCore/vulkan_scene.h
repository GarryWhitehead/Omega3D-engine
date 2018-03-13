#pragma once
#include "VulkanCore/vulkan_core.h"
#include "VulkanCore/vulkan_utility.h"

class VulkanModel;
class VulkanTerrain;
class CameraSystem;

class VulkanScene : public VulkanCore
{

public:

	static constexpr VkClearColorValue CLEAR_COLOR = { 0.0f, 0.2f, 0.2f, 1.0f };

	VulkanScene();
	~VulkanScene();

	void Init();
	void Update(CameraSystem *camera);
	void RenderFrame();
	TextureInfo InitDepthImage();
	void PrepareRenderpass();
	void PrepareFrameBuffers();
	

	friend class VulkanModel;
	friend class VulkanTerrain;
	friend class VulkanShadow;
	friend class VulkanUtility;

protected:

	// composition elements
	VulkanModel *p_vulkanModel;
	VulkanTerrain *p_vulkanTerrain;

	VkFormat m_depthImageFormat;
	VulkanUtility::ViewPortInfo m_viewport;
	VkCommandPool m_cmdPool;
	VkRenderPass m_renderpass;
	std::vector<VkFramebuffer> m_frameBuffer;
	TextureInfo m_depthImage;

	bool vk_prepared;
};

