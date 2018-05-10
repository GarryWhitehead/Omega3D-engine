#pragma once
#include "VulkanCore/VulkanModule.h"
#include "ComponentManagers/MeshComponentManager.h"

class VulkanEngine;
class CameraSystem;

const int MAX_BONES = 64;
const int MAX_VERTEX_BONES = 4;
const int MAX_MODELS = 256;

enum class MaterialTexture
{
	TEX_DIFF = 1 << 0,
	TEX_NORM = 1 << 1,
	TEX_PBR =  1 << 2,
	TEX_NONE = 1 << 3
};

class VulkanModel: public VulkanModule
{

public:

	// generic vertex layout used (hopefully) by all model-based vertex shaders
	struct ModelVertex
	{
		VkVertexInputBindingDescription ModelVertex::GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 6> ModelVertex::GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 colour;
		float boneWeigthts[MAX_VERTEX_BONES];
		uint32_t boneId[MAX_VERTEX_BONES];
	};

	// bases for mesh/index data
	struct MeshInfo
	{
		// index into material buffer for each mesh
		uint32_t materialIndex;

		// index data for each mesh
		uint32_t indexBase;
		uint32_t indexCount;
	};

	struct ModelInfo
	{
		uint32_t verticesOffset;
		uint32_t indicesOffset;

		std::vector<MeshInfo> meshes;
	};

	// material data in format used for push constants
	struct MaterialProperties
	{
		glm::vec4 color;
		float roughness;
		float metallic; 
	};

	struct Material
	{
		Material() : matTypes(0) {}

		std::string name;
		MaterialProperties properties;
		uint32_t matTypes;

		struct Textures
		{
			TextureInfo diffuse;
			TextureInfo specular;		// same as metallic
			TextureInfo normal;
			TextureInfo roughness;		// PBR
			TextureInfo metallic;		// PBR
			TextureInfo nomap;			// dummy texture
		}  texture;

		VkDescriptorSet descrSet;
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	struct AnimationSets
	{
		VulkanUtility::DescriptorInfo descriptors;
	};

	struct AnimationInfo
	{
		AnimationSets mesh;
		AnimationSets material;

		VkPipelineLayout pipelineLayout;
		struct MaterialPipelines
		{
			VulkanUtility::PipeLlineInfo diffNorm;
			VulkanUtility::PipeLlineInfo diff;
			VulkanUtility::PipeLlineInfo pbr;
			VulkanUtility::PipeLlineInfo nomap;
		} matPipelines;

		struct MaterialDeccriptors
		{
			VkDescriptorSetLayout diffLayout;
			VkDescriptorSetLayout nomapLayout;
			VkDescriptorSetLayout diffnormLayout;
			VkDescriptorSetLayout pbrLayout;
		} descriptors;

		std::array<VkPipelineShaderStageCreateInfo, 2> shader;
	};

	struct SsboLayout
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		glm::mat4 modelMatrix[MAX_MODELS];
		glm::mat4 boneTransform[MAX_BONES];
	};

	VulkanModel(VulkanEngine *engine, VulkanUtility *utility);
	~VulkanModel();

	void Init();
	void Update(CameraSystem *camera);
	void Destroy() override;

	void PrepareMeshDescriptorSet();
	void PrepareMaterialDescriptorPool(uint32_t materialCount);
	void PrepareMaterialDescriptorLayouts();
	void PrepareMaterialDescriptorSets(Material *material);
	void PreparePipeline();
	void GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, VkDescriptorSet set, VkPipelineLayout layout, VkPipeline pipeline);
	void CreateBuffers();
	void PrepareSsboBuffer();

	// mesh-material vertex preperation functions
	void ProcessMeshes();
	void ProcessMaterials();
	TextureInfo LoadMaterialTexture(MeshComponentManager::OMFMaterial &material, MaterialType type);
	uint8_t FindMaterialIndex(std::string matName);
	void AddPipelineDataToModels();

	// helper functions
	ModelInfo& RequestModelInfo(const uint32_t index) { return m_modelInfo[index]; }
	VkBuffer& GetIndexBuffer()  { return m_indexBuffer.buffer; }
	VkBuffer& GetVertexBuffer() { return m_vertexBuffer.buffer; }

	friend class VulkanEngine;


private:

	VulkanEngine *p_vkEngine;

	// buffer info for the "mega" buffer which holds all the models and are referenced via offsets
	BufferData m_vertexBuffer;
	BufferData m_indexBuffer;
	BufferData m_ssboBuffer;

	AnimationInfo m_animInfo;

	std::vector<ModelInfo> m_modelInfo;

	// material data used by pipline
	std::vector<Material> m_materials;

};

