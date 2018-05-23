#pragma once
#include <vector>
#include <unordered_map>
#include "VulkanCore/vulkan_core.h"
#include "VulkanCore/VulkanTexture.h"
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
class World;

enum class VkModId
{
	VKMOD_TERRAIN_ID,
	VKMOD_SHADOW_ID,
	VKMOD_MODEL_ID,
	VKMOD_DEFERRED_ID,
	VKMOD_PBR_ID,
	VKMOD_IBL_ID,
	VKMOD_SKYBOX_ID,
	VKMOD_WATER_ID
};

class VulkanEngine : public VulkanCore
{

public:

	static constexpr VkClearColorValue CLEAR_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	VulkanEngine(GLFWwindow *window);
	~VulkanEngine();

	void Init(World *world);
	void Update(int acc_time);
	void Render();

	void RegisterVulkanModules(std::vector<VkModId> modules);
	void RegisterGraphicsSystem(GraphicsSystem *graphics) { assert(graphics != nullptr); p_graphicsSystem = graphics; }
	
	template<typename T>
	T* VkModule(VkModId id);
	bool hasModule(VkModId id);

	// helper functions
	VkDevice GetDevice() const { return m_device.device; }
	VkCommandPool GetCmdPool() const { return m_cmdPool; }
	uint32_t GetGraphQueueIndex() const { return m_queue.graphIndex; }
	uint32_t GetComputeQueueIndex() const { return m_queue.computeIndex; }
	VkQueue GetComputeQueue() const { return m_queue.computeQueue; }
	VkCommandPool GetComputeCmdPool() const { return m_computeCmdPool; }
	

	friend class VulkanModel;
	friend class VulkanTerrain;
	friend class VulkanShadow;
	friend class VulkanUtility;
	friend class VulkanAnimation;
	friend class VulkanDeferred;
	friend class VulkanIBL;
	friend class VulkanPBR;
	friend class VulkanSkybox;
	friend class VulkanWater;
	friend class VulkanTexture;

protected:

	void SubmitFrame();
	void DrawScene();
	void RenderScene(VkCommandBuffer cmdBuffer, VkDescriptorSet set = VK_NULL_HANDLE, VkPipelineLayout layout = VK_NULL_HANDLE, VkPipeline pipeline = VK_NULL_HANDLE);

	World *p_world;

	VulkanUtility *vkUtility;
	GraphicsSystem *p_graphicsSystem;

	std::unordered_map<VkModId, VulkanModule*> m_vkModules;

	VulkanUtility::ViewPortInfo m_viewport;
	VkCommandPool m_cmdPool;
	VkCommandPool m_computeCmdPool;

	VkCommandBuffer m_offscreenCmdBuffer;
	std::vector<VkCommandBuffer> m_cmdBuffers;
	bool drawStateChanged;
	bool vk_prepared;
};

template<typename T>
T* VulkanEngine::VkModule(VkModId id)
{
	T* mod = (T*)m_vkModules[id]; //static_cast<T*>(m_vkModules[id]);
	assert(mod != nullptr);
	return mod;
}
