#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "VulkanCore/vulkan_core.h"
#include "VulkanCore/VulkanTexture.h"
#include "VulkanCore/vulkan_utility.h"

class VulkanShadow;
class VulkanModel;
class VulkanAnimation;
class VulkanDeferred;
class VulkanTerrain;
class CameraSystem;
class VkMemoryManager;
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
	void RenderScene(VkCommandBuffer cmdBuffer, VkDescriptorSet set = VK_NULL_HANDLE, VkPipelineLayout layout = VK_NULL_HANDLE, VkPipeline pipeline = VK_NULL_HANDLE);

	void RegisterVulkanModules(std::vector<VkModId> modules);
	void RegisterGraphicsSystem(GraphicsSystem *graphics) { assert(graphics != nullptr); p_graphicsSystem = graphics; }
	
	template<typename T>
	T* VkModule();

	template<typename T>
	bool hasModule();

	// helper (getter) functions
	VkDevice GetDevice() const { return m_device.device; }
	VkPhysicalDevice GetPhysicalDevice() const { return m_device.physDevice; }
	uint32_t GetSurfaceExtentW() { return m_surface.extent.width; }
	uint32_t GetSurfaceExtentH() { return m_surface.extent.height; }
	VkCommandPool GetCmdPool() const { return m_cmdPool; }
	uint32_t GetGraphQueueIndex() const { return m_queue.graphIndex; }
	VkQueue GetGraphQueue() const { return m_queue.graphQueue; }
	uint32_t GetComputeQueueIndex() const { return m_queue.computeIndex; }
	VkQueue GetComputeQueue() const { return m_queue.computeQueue; }
	VkCommandPool GetComputeCmdPool() const { return m_computeCmdPool; }
	VkViewport GetViewPort() const { return m_viewport.viewPort; }
	VkRect2D GetScissor() const { return m_viewport.scissor; }
	uint32_t GetSwapChainImageCount() const { return m_swapchain.imageCount; }
	VkImageView GetImageView(const uint32_t index) const { return m_imageView.images[index]; }
	VkFormat GetSurfaceFormat() const { return m_surface.format.format; }

	World* GetCurrentWorld() { return p_world; }
	GraphicsSystem* RequestGraphicsSystem() { assert(p_graphicsSystem != nullptr); return p_graphicsSystem; }

	friend class VulkanUtility;


protected:

	void SubmitFrame();
	void DrawScene();

	World *p_world;
	VkMemoryManager *p_vkMemory;

	VulkanUtility *vkUtility;
	GraphicsSystem *p_graphicsSystem;

	std::unordered_map<std::type_index, VulkanModule*> m_vkModules;

	VulkanUtility::ViewPortInfo m_viewport;
	VkCommandPool m_cmdPool;
	VkCommandPool m_computeCmdPool;

	VkCommandBuffer m_offscreenCmdBuffer;
	std::vector<VkCommandBuffer> m_cmdBuffers;
	bool drawStateChanged;
	bool vk_prepared;
};

template <typename T>
T* VulkanEngine::VkModule()
{
	std::type_index index(typeid(T));

	T* mod = (T*)m_vkModules[index]; //static_cast<T*>(m_vkModules[id]);
	assert(mod != nullptr);
	return mod;
}

template <typename T>
bool VulkanEngine::hasModule()
{
	std::type_index index(typeid(T));

	auto& iter = m_vkModules.find(index);
	return(iter != m_vkModules.end());
}
