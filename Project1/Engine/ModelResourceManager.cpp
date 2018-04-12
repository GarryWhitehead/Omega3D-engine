#include "ModelResourceManager.h"
#include "utility/file_log.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_model.h"
#include "VulkanCore/VulkanAnimation.h"
#include "ComponentManagers/MeshComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"

ModelResourceManager::ModelResourceManager()
{
}

ModelResourceManager::ModelResourceManager(VulkanEngine *engine) :
	p_vkEngine(engine)
{
	p_vulkanModel = p_vkEngine->AssociateWithVulkanModel(this);
	p_vulkanAnim = p_vkEngine->AssociateWithVulkanAnimation(this);
	
}

ModelResourceManager::~ModelResourceManager()
{
}

void ModelResourceManager::RegisterVulkanEngine(VulkanEngine *engine)
{
	p_vkEngine = engine; 
	
	if (p_vulkanModel == nullptr) {
		p_vulkanModel = p_vkEngine->AssociateWithVulkanModel(this);
	}

	if (p_vulkanAnim == nullptr) {
		p_vulkanAnim = p_vkEngine->AssociateWithVulkanAnimation(this);
	}
}

void ModelResourceManager::DownloadMeshData(MeshComponentManager *meshManager)
{
	// clear data from previous update
	m_updatedStaticMeshInd.clear();
	m_updatedAnimMeshInd.clear();

	meshManager->DownloadMeshIndicesData(m_updatedStaticMeshInd, m_updatedAnimMeshInd);
}

void ModelResourceManager::DownloadTransformData(TransformComponentManager* transManager)
{
	// clear data from previous update
	m_updatedStaticTransform.clear();
	m_updatedAnimTransform.clear();

	transManager->DownloadWorldTransformData(m_updatedStaticTransform, m_updatedAnimTransform);
}

void ModelResourceManager::PrepareStaticModelResources(std::vector<std::string>& filenames)
{
	// start by importing all static obj models associated with this space
	ImportModels(filenames);

	// create uniform buffer for camera perspective info 
	p_vulkanModel->PrepareUBOBuffer();

	// prepare descriptor sets for meshes and materials
	p_vulkanModel->PrepareMeshDescriptorSet();
	p_vulkanModel->PrespareMaterialDescriptorPool(m_staticMaterialCount);
	
	for (auto& model : m_models) {

		p_vulkanModel->PrepareMaterialDescriptorSet(&model);
	}

	// prepare mesh and material pipelines
	p_vulkanModel->PrepareModelPipelineWithMaterial();
	p_vulkanModel->PrepareModelPipelineWithoutMaterial();

	// add relevant descriptor set and pipeline info to each model
	AddPipelineDataToModels();
}

void ModelResourceManager::PrepareAnimatedModelResources(std::vector<std::string>& colladaFilenames)
{
	ImportColladaModels(colladaFilenames);

	// create uniform buffer for camera perspective info 
	p_vulkanAnim->PrepareUBOBuffer();

	// prepare descriptor sets for meshes and materials
	p_vulkanAnim->PrepareDescriptorSet(m_colladaModels[0].m_matTexture);
	p_vulkanAnim->PreparePipeline();
}

void ModelResourceManager::ImportModels(std::vector<std::string>& filenames)
{
	uint32_t totalVertexSize = 0;
	uint32_t totalIndexSize = 0;

	
	for (auto& name : filenames) {

		ModelInfo model;
		model.LoadModel(name, p_vkEngine, p_vkEngine->m_cmdPool);
		m_models.push_back(model);

		// total buffer size required for static meshes
		totalVertexSize += sizeof(ModelInfo::ModelVertex) * model.vertices.size();
		totalIndexSize += sizeof(uint32_t) * model.indices.size();

		*g_filelog << "Importing model data from " << name << "......... Sucessfully imported into world space!";
	}

	// map vertex and index data to out mega buffer hosted on device
	p_vulkanModel->CreateVertexBuffer(totalVertexSize);
	p_vulkanModel->CreateIndexBuffer(totalIndexSize);

	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;

	for (auto &model : m_models) {

		p_vulkanModel->MapDataToBuffers(&model, vertexOffset, indexOffset);
		
		// offset into vertex buffer
		model.vertexBuffer.offset = vertexOffset;
		vertexOffset += model.vertexBuffer.size;
		
		// offset into index buffer
		model.indexBuffer.offset = indexOffset;
		indexOffset += model.indexBuffer.size;
		
		// also calculate the total number of materials contained within all models
		m_staticMaterialCount += model.materialData.size() + 1;
	}
}

void ModelResourceManager::AddPipelineDataToModels()
{
	for (auto& model : m_models) {
		
		if (!model.materialData.empty()) {
		
			for (uint32_t c = 0; c < model.meshData.size(); ++c) {

				// descriptor set for mesh and material
				model.meshData[c].descrSets.push_back(p_vulkanModel->m_meshDescrInfo.set);
				model.meshData[c].descrSets.push_back(model.meshData[c].material->descrSet);

				// pipeline info
				model.meshData[c].pipelineLayout = p_vulkanModel->m_matPipeline.layout;
				model.meshData[c].pipeline = p_vulkanModel->m_matPipeline.pipeline;
			}
		}
		else {
			// pipelines and descriptor sets for meshes with no material information
			for (uint32_t c = 0; c < model.meshData.size(); ++c) {
				
				model.meshData[c].descrSets.push_back(p_vulkanModel->m_meshDescrInfo.set);

				// pipeline info
				model.meshData[c].pipelineLayout = p_vulkanModel->m_noMatPipeline.layout;
				model.meshData[c].pipeline = p_vulkanModel->m_noMatPipeline.pipeline;
			}
		}
	}
}

void ModelResourceManager::ImportColladaModels(std::vector<std::string>& filenames)
{
	uint32_t totalVertexSize = 0;
	uint32_t totalIndexSize = 0;

	for (auto& name : filenames) {

		ColladaModelInfo model;
		model.ImportFile(name, p_vkEngine, p_vkEngine->m_cmdPool);
		m_colladaModels.push_back(model);

		// total buffer size required for static meshes
		totalVertexSize += sizeof(ColladaModelInfo::ModelVertex) * model.p_scene->meshData[0].numPositions();
		totalIndexSize += sizeof(uint32_t) * model.p_scene->meshData[0].totalIndices();

		*g_filelog << "Importing collada model data from " << name << "......... Sucessfully imported into world space!";
	}

	p_vulkanAnim->CreateVertexBuffer(totalVertexSize);
	p_vulkanAnim->CreateIndexBuffer(totalIndexSize);

	uint32_t vertexOffset = 0;
	uint32_t indexOffset = 0;

	for (auto &model : m_colladaModels) {

		p_vulkanAnim->MapDataToBuffers(&model, vertexOffset, indexOffset);

		// offset into vertex buffer
		model.vertexBuffer.offset = vertexOffset;
		vertexOffset += model.vertexBuffer.size;

		// offset into index buffer
		model.indexBuffer.offset = indexOffset;
		indexOffset += model.indexBuffer.size;

		// also calculate the total number of materials contained within all models
		m_animMaterialCount += model.m_materials.size();
	}
}

glm::mat4 ModelResourceManager::GetUpdatedTransform(uint32_t index, ModelType type)
{
	glm::mat4 mat(1.0f);
	if (type == ModelType::MODEL_STATIC) {
		
		if (m_updatedStaticTransform.empty()) {
			mat = glm::mat4(1.0f);
		}
		else {
			mat = m_updatedStaticTransform[index];
		}
	}
	else if (type == ModelType::MODEL_ANIMATED) {
		if (m_updatedAnimTransform.empty()) {
			mat = glm::mat4(1.0f);
		}
		else {
			mat = m_updatedAnimTransform[index];
		}
	}
	return mat;
}