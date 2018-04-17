#pragma once
#include <vector>
#include "VulkanCore/ModelInfo.h"
#include "VulkanCore/ColladaModelInfo.h"

enum class ModelType;
class VulkanEngine;
class VulkanModel;
class VulkanAnimation;
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
	void PrepareStaticModelResources(std::vector<std::string>& filenames);
	void PrepareAnimatedModelResources(std::vector<std::string>& colladaFilenames);
	void RegisterVulkanEngine(VulkanEngine *engine);

	// helper functions
	glm::mat4 GetUpdatedTransform(uint32_t index, ModelType type);

	friend class VulkanModel;
	friend class VulkanAnimation;

private:

	void ImportModels(std::vector<std::string>& filenames);
	void AddPipelineDataToModels();
	void PrepareMaterialData();

	// collada models
	void ImportColladaModels(std::vector<std::string>& filenames);

	//all the model data stored in one large chunck of mem
	std::vector<ModelInfo> m_models;

	// collaada models
	std::vector<ColladaModelInfo> m_colladaModels;

	VulkanEngine *p_vkEngine;
	VulkanModel *p_vulkanModel;
	VulkanAnimation *p_vulkanAnim;

	uint32_t m_staticMaterialCount;
	uint32_t m_animMaterialCount;

	// mesh and transform data stored for rendering purposes
	std::vector<uint32_t> m_updatedStaticMeshInd;
	std::vector<glm::mat4> m_updatedStaticTransform;

	std::vector<uint32_t> m_updatedAnimMeshInd;
	std::vector<glm::mat4> m_updatedAnimTransform;
};

