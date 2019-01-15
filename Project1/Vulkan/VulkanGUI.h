#pragma once
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/MemoryAllocator.h"

class VkDescriptors;
class VulkanTexture;
class VulkanRenderPass;
class VulkanEngine;

class VulkanGUI
{

public:

	// settings are public and can be altered by all vulkan modules
	struct GUISettingsInfo
	{
		GUISettingsInfo() :
			amplitude(40.0f),
			terrainType(0),
			wireframe(false),		// default settings
			showFog(false),
			lights(true),
			exposure(1.0f),
			gamma(2.2f)
		{}

		int terrainType;

		// debugging
		bool wireframe;
		bool debugShadow;

		// environmental control
		bool showFog;
		bool lights;
		
		// water attributes
		float amplitude;
		float choppiness;

		//terrain attributes
		float displacement;
		float tesselation;
		float edgeFactor;

		// tone mapping
		float exposure;
		float gamma;

	} m_guiSettings;

	struct PushConstant
	{
		glm::vec2 scale;
	};

	VulkanGUI(VulkanEngine *engine);
	~VulkanGUI();

	void Init(MemoryAllocator *p_vkMemory);
	void Update();

	void SetupGUI(MemoryAllocator *p_vkMemory);
	void PrepareDescriptors();
	void PreparePipeline();
	void NewFrame();
	void UpdateBuffers(MemoryAllocator *p_vkMemory);
	void GenerateCmdBuffer(VkCommandBuffer cmdBuffer, MemoryAllocator *p_vkMemory);
	void PrepareGUIVertex(VkVertexInputBindingDescription& bindDescr, std::array<VkVertexInputAttributeDescription, 3>& attrDescr);

private:

	void Destroy();

	VulkanEngine *p_vkEngine;

	// pipeline data
	VulkanTexture *m_fontImage;
	VkDescriptors *m_descriptors;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;

	VulkanUtility::PipeLlineInfo m_pipelineInfo;
	MemoryAllocator::MemorySegment m_vertices;
	MemoryAllocator::MemorySegment m_indices;

	// keep track of vertex/index sizes for buffer management
	uint32_t m_vertCount;
	uint32_t m_indCount;
};

