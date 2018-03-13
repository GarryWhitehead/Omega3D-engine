#pragma once
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm.hpp"

#include "gtc/type_ptr.hpp"
#include "VulkanCore/vulkan_utility.h"
#include "VulkanCore/vulkan_tools.h"
#include "Objector/objector.h"
#include "Objector/MaterialParser.h"
#include <string>
#include <vector>
#include <array>
#include <memory>

struct TextureInfo;
struct objMaterial;
class CameraSystem;
class VulkanScene;

class VulkanModel : public VulkanUtility
{
public:

	struct Vertex
	{
		VkVertexInputBindingDescription Vertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 4> Vertex::GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 colour;
	};

	struct UboLayout
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix;
		glm::vec4 lightPos;
	};

	struct MaterialProperties
	{
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 ambient;
		float opacity;
	};

	struct Material
	{
		std::string name;
		int id;
		MaterialProperties properties;
		TextureInfo diffuse;
		TextureInfo specular;

		// each material has its own descriptort set and pipeline
		VkDescriptorSet descrSet;
		VkPipeline pipeline;
	};

	struct MeshData
	{
		int32_t indexBase;
		uint32_t indexCount;

		Material *material;
	};

	struct DescriptorInfo 
	{
		VkDescriptorSetLayout layout;
	};

	VulkanModel();
	VulkanModel(VulkanScene *vulkanScene);
	~VulkanModel();

	void InitVulkanModel();
	void Update(CameraSystem *camera);
	void LoadScene(std::string filename);
	void LoadMeshes();
	void LoadMaterials();
	TextureInfo LoadMaterialTexture(objMaterial& material, objTextureType type);
	void PrepareModelDescriptorSets();
	void PrepareModelPipelineWithMaterial();
	void PrepareModelPipelineWithoutMaterial();
	void GenerateModelCmdBuffer();
	void PrepareUBOBuffer();
	void MapVertexBufferToMemory();
	void MapIndexBufferToMemory();

	friend class VulkanScene;

private:

	VulkanScene *p_vulkanScene;
	Objector objector;

	std::vector<MeshData> m_meshData;
	std::vector<Material> m_materialData;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	Objector::ModelInfo *m_model;

	VkDescriptorPool m_descrPool;
	VkDescriptorSet m_sceneDescrSet;
	DescriptorInfo m_sceneDescr;
	DescriptorInfo m_materialDescr;

	std::vector<VkCommandBuffer> m_cmdBuffer;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;
	
	PipeLlineInfo m_matPipeline;
	PipeLlineInfo m_noMatPipeline;

	BufferData m_indexBuffer;
	BufferData m_vertexBuffer;
	BufferData m_uboBuffer;

	bool vk_prepared;

};

