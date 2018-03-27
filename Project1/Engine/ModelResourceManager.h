#pragma once
#include <vector>
#include "VulkanCore/ModelInfo.h"

class VulkanEngine;
class VulkanModel;
class MeshComponentManager;
class TransformComponentManager;

class ModelResourceManager
{

public:

	ModelResourceManager();
	ModelResourceManager(VulkanEngine *engine);
	~ModelResourceManager();

	void DownloadMeshData(MeshComponentManager *meshManager);
	void DownloadTransformData(TransformComponentManager* transManager);
	void PrepareModelResources(std::vector<std::string>& filenames);
	void RegisterVulkanEngine(VulkanEngine *engine);
	
	friend class VulkanModel;

private:

	void ImportModels(std::vector<std::string>& filenames);
	void AddPipelineDataToModels();

	//all the model data stored in one large chunck of mem
	std::vector<ModelInfo> m_models;

	VulkanEngine *p_vkEngine;
	VulkanModel *p_vulkanModel;

	uint32_t m_materialCount;

	// mesh and transform data stored for rendering purposes
	std::vector<uint32_t> m_updatedMeshInd;
	std::vector<glm::mat4> m_updatedTransform;
};

