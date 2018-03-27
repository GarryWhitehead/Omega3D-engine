#include "ModelResourceManager.h"
#include "utility/file_log.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/vulkan_model.h"
#include "ComponentManagers/MeshComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"

ModelResourceManager::ModelResourceManager()
{
}

ModelResourceManager::ModelResourceManager(VulkanEngine *engine) :
	p_vkEngine(engine)
{
	p_vulkanModel = p_vkEngine->RegisterModelResourceManager(this);
}

ModelResourceManager::~ModelResourceManager()
{
}

void ModelResourceManager::RegisterVulkanEngine(VulkanEngine *engine)
{
	p_vkEngine = engine; 
	
	if (p_vulkanModel == nullptr) {
		p_vulkanModel = p_vkEngine->RegisterModelResourceManager(this);
	}
}

void ModelResourceManager::DownloadMeshData(MeshComponentManager *meshManager)
{
	// clear data from previous update
	m_updatedMeshInd.clear();

	meshManager->DownloadMeshIndicesData(m_updatedMeshInd);
}

void ModelResourceManager::DownloadTransformData(TransformComponentManager* transManager)
{
	// clear data from previous update
	m_updatedTransform.clear();

	transManager->DownloadWorldTransformData(m_updatedTransform);
}

void ModelResourceManager::PrepareModelResources(std::vector<std::string>& filenames)
{
	// start by importing all models associated with this space
	ImportModels(filenames);

	// create uniform buffer for camera perspective info 
	p_vulkanModel->PrepareUBOBuffer();

	// prepare descriptor sets for meshes and materials
	p_vulkanModel->PrepareMeshDescriptorSet();
	p_vulkanModel->PrespareMaterialDescriptorPool(m_materialCount);
	
	for (auto& model : m_models) {

		p_vulkanModel->PrepareMaterialDescriptorSet(&model);
	}

	// prepare mesh and material pipelines
	p_vulkanModel->PrepareModelPipelineWithMaterial();
	p_vulkanModel->PrepareModelPipelineWithoutMaterial();

	// add relevant descriptor set and pipeline info to each model
	AddPipelineDataToModels();
}

void ModelResourceManager::ImportModels(std::vector<std::string>& filenames)
{
	uint32_t totalVertexSize = 0;
	uint32_t totalIndexSize = 0;

	
	for (auto& name : filenames) {

		ModelInfo model;
		model.LoadModel(name, p_vkEngine->m_device.device, p_vkEngine->m_cmdPool);
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
		m_materialCount += model.materialData.size() + 1;
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