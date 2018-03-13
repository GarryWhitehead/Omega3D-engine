#pragma once
#include "vulkan_utility.h"
#include <array>

class VulkanScene;

class VulkanShadow : public VulkanUtility
{

public:

	const uint32_t SHADOWMAP_WIDTH = 400;
	const uint32_t SHADOWMAP_HEIGHT = 400;
	const int CSM_COUNT = 4;

	

	VulkanShadow();
	VulkanShadow(VulkanScene* vulkanScene);
	~VulkanShadow();

	void PrepareDepthImage();
	void GenerateCascadeCmdBuffer();

private:

	struct CascadeInfo
	{
		std::array<VkImageView, 4> imageView;
		std::array<VkFramebuffer, 4> frameBuffer;
		VkCommandBuffer cmdBuffer;
	} m_cascadeInfo;

	TextureInfo m_depthInfo;

	VulkanScene *p_vulkanScene;
};

