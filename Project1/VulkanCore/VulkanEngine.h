#pragma once
#include <vector>
#include <unordered_map>
#include "VulkanCore/vulkan_core.h"
#include "VulkanCore/vulkan_utility.h"

class VulkanShadow;
class VulkanModel;
class VulkanAnimation;
class VulkanDeferred;
class VulkanTerrain;
class CameraSystem;
class ModelResourceManager;
class VulkanModule;
class GraphicsSystem;

enum class VkModId
{
	VKMOD_TERRAIN_ID,
	VKMOD_SHADOW_ID,
	VKMOD_MODEL_ID,
	VKMOD_ANIM_ID,
	VKMOD_DEFERRED_ID
};

class VulkanEngine : public VulkanCore
{

public:

	static constexpr VkClearColorValue CLEAR_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	VulkanEngine(GLFWwindow *window);
	~VulkanEngine();

	void Init();
	void Update(CameraSystem *camera);
	void Render();

	VulkanModel* AssociateWithVulkanModel(ModelResourceManager* manager);
	VulkanAnimation*  AssociateWithVulkanAnimation(ModelResourceManager* manager);

	void RegisterVulkanModules(std::vector<VkModId> modules);
	void RegisterGraphicsSystem(GraphicsSystem *graphics) { assert(graphics != nullptr); p_graphicsSystem = graphics; }
	
	template<typename T>
	T* VkModule(VkModId id);

	friend class VulkanModel;
	friend class VulkanTerrain;
	friend class VulkanShadow;
	friend class VulkanUtility;
	friend class VulkanAnimation;
	friend class VulkanDeferred;
	friend class ModelResourceManager;

protected:

	void SubmitFrame();
	TextureInfo InitDepthImage();
	void PrepareRenderpass();
	void PrepareFrameBuffers();
	void DrawScene();
	void RenderScene(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline);

	VulkanUtility *vkUtility;
	GraphicsSystem *p_graphicsSystem;

	std::unordered_map<VkModId, VulkanModule*> m_vkModules;

	VkFormat m_depthImageFormat;
	VulkanUtility::ViewPortInfo m_viewport;
	VkCommandPool m_cmdPool;
	VkRenderPass m_renderpass;
	std::vector<VkFramebuffer> m_frameBuffer;
	VkCommandBuffer m_offscreenCmdBuffer;
	std::vector<VkCommandBuffer> m_sceneCmdBuffer;
	TextureInfo m_depthImage;

	bool drawStateChanged;
	bool vk_prepared;
};

template<typename T>
T* VulkanEngine::VkModule(VkModId id)
{
	T* mod = static_cast<T*>(m_vkModules[id]);
	assert(mod != nullptr);
	return mod;
}
