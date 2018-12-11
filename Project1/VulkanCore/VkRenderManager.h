#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/vulkan_utility.h"
#include "utility/message_handler.h"

#include "volk.h"

// forward class declerations
class VulkanDevice;
class VulkanInstance;
class VulkanRenderPass;
class VulkanTexture;
class VkSwapChain;
class ValidationLayers;
class VulkanShadow;
class VulkanModel;
class VulkanDeferred;
class VulkanTerrain;
class VkMemoryManager;
class VulkanGUI;
class VulkanModule;
class World;

// enum used for specifying which modules to optional include during generation of the world engine
enum class VkModId
{
	VKMOD_TERRAIN_ID,
	VKMOD_MODEL_ID,
	VKMOD_SKYBOX_ID,
	VKMOD_WATER_ID,
};

class VulkanEngine
{

public:

	static constexpr VkClearColorValue CLEAR_COLOR = { 0.0f, 0.0f, 0.0f, 1.0f };		// used by all command buffers
	
	VulkanEngine(GLFWwindow *window, MessageHandler *msgHandler);
	~VulkanEngine();

	void Init(World *world);
	void Update(int acc_time);

	// clean up functions
	void DestroyPresentation();
	void PrepareNewSwapFrame();

	// rendering functions - TODO: make these function names a little more obvious and transparent!
	void Render();
	void RenderScene(VkCommandBuffer cmdBuffer, bool drawShadow = false);

	// message handling function
	void OnNotify(Message& msg);

	// can either render with pipeline info passed through arguments - in the case of rendering shadows, etc. or using modules own pipeline 
	void PrepareFinalFrameBuffer(bool prepareFrameBufferOnly);
	void GenerateFinalCmdBuffer();

	void RegisterVulkanModules(std::vector<VkModId> modules);
	
	// Request the poinetr of a vulkan module held by the engine. Throws an exception if no requested module is found
	template<typename T>
	T* VkModule();

	// check whether this particular world has a certain vulkan module
	template<typename T>
	bool hasModule();

	// helper (getter) functions
	VkDevice GetDevice() const;
	VkPhysicalDevice GetPhysicalDevice() const;
	uint32_t GetSurfaceExtentW();
	uint32_t GetSurfaceExtentH();
	VkCommandPool GetCmdPool() const { return m_cmdPool; }
	uint32_t GetGraphQueueIndex() const;
	VkQueue GetGraphQueue() const;
	uint32_t GetComputeQueueIndex() const;
	VkQueue GetComputeQueue() const;
	VkCommandPool GetComputeCmdPool() const { return m_computeCmdPool; }
	VkViewport GetViewPort() const { return m_viewport.viewPort; }
	VkRect2D GetScissor() const { return m_viewport.scissor; }
	uint32_t GetSwapChainImageCount() const;
	VkImageView GetImageView(const uint32_t index) const;
	VkFormat GetSurfaceFormat() const;

	World* GetCurrentWorld() { return p_world; }
	VkRenderPass GetFinalRenderPass() const;
	void ClearDrawState() { drawStateChanged = true; }

	// setings
	bool drawWireframe() const;
	uint32_t drawFog() const;		// return an int instead of a bool due to the shader setup 
	bool displayLights() const;
	int terrainType() const;
	float waveAmplitude() const;
	float choppyFactor() const;
	float tesselationFactor() const;
	float displacementFactor() const;
	float tessEdgeSize() const;
	float exposureSetting() const;
	float gammaSetting() const;
	bool debugShadowSetting() const;

protected:

	void Destroy();

	// private functions for message system
	std::function<void(Message)> NotifyResponse();
	void SendMessage(Message msg);

	// to avoid confusion with the other draw functions, these are kept private, only Render() should be called for scene drawing
	void SubmitFrame();
	void DrawScene();

	// pointers to objects that the engine is highly integradted with
	MessageHandler *p_message;
	World *p_world;
	VkMemoryManager *p_vkMemory;
	VulkanGUI *p_vkGUI;

	// pointers to core components
	VulkanDevice *p_vkDevice;
	VulkanInstance *p_vkInstance;
	VkSwapChain *p_vkSwapChain;

	// a map of all the vk modules linked to this world instance 
	std::unordered_map<std::type_index, VulkanModule*> m_vkModules;

	VulkanUtility::ViewPortInfo m_viewport;
	VkCommandPool m_cmdPool;						// just one command pool for both graphics and compute for all modules/dependencies
	VkCommandPool m_computeCmdPool;

	// frame buffer and command buffer data for final pass
	VulkanRenderPass *m_renderpass;
	std::vector<VkFramebuffer> m_frameBuffers;
	std::vector<VkCommandBuffer> m_cmdBuffers;
	VulkanTexture *m_depthImage;

	// states for drawing, displaying info, etc.
	bool drawStateChanged;
	bool vk_prepared;
	bool displayGUI;
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
