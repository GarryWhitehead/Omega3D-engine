#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm.hpp"

#include <gtc/matrix_transform.hpp>
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/ModelInfo.h"
#include <string>
#include <vector>
#include <array>
#include <memory>

class CameraSystem;
class VulkanEngine;
class VulkanShadow;
struct ModelInfo;
class ModelResourceManager;

class VulkanModel : public VulkanModule
{
public:

	struct UboLayout
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
	};

	VulkanModel(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanModel();

	void Update(CameraSystem *camera);
	void Destroy() override;

	void PrepareMeshDescriptorSet();
	void PrespareMaterialDescriptorPool(uint32_t materialCount);
	void PrepareMaterialDescriptorSet(ModelInfo *model);
	void PrepareModelPipelineWithMaterial();
	void PrepareModelPipelineWithoutMaterial();
	void GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline = VK_NULL_HANDLE);
	void PrepareUBOBuffer();
	void CreateVertexBuffer(uint32_t bufferSize);
	void CreateIndexBuffer(uint32_t bufferSize);
	void MapDataToBuffers(ModelInfo *model, uint32_t vertexOffset, uint32_t indexOffset);

	friend class VulkanEngine;
	friend class VulkanShadow;
	friend class ModelResourceManager;

private:

	VulkanEngine *p_vkEngine;
	ModelResourceManager *p_modelManager;

	BufferData m_vertexBuffer;
	BufferData m_indexBuffer;

	VulkanUtility::DescriptorInfo m_meshDescrInfo;
	VulkanUtility::DescriptorInfo m_materialDescrInfo;

	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;
	
	VulkanUtility::PipeLlineInfo m_matPipeline;
	VulkanUtility::PipeLlineInfo m_noMatPipeline;

	BufferData m_uboBuffer;

	bool vk_prepared;

};

