#pragma once
#include "VulkanCore/VulkanModule.h"
#include "VulkanCore/vulkan_tools.h"
#include "VulkanCore/MemoryAllocator.h"
#include "ComponentManagers/MeshComponentManager.h"
#include <vector>

class VulkanEngine;
class VkDescriptors;
class VulkanTexture;
class CameraSystem;
class MemoryAllocator;

enum class MaterialTexture
{
	TEX_DIFF =			1 << 0,
	TEX_NORM =			1 << 1,
	TEX_ROUGHNESS =		1 << 2,
	TEX_METALLIC =		1 << 3,
	TEX_AO =			1 << 4,
	TEX_NONE =			1 << 5
};

class VulkanModel: public VulkanModule
{

public:

	static const int MAX_BONES = 64;
	static const int MAX_VERTEX_BONES = 4;
	static const int MAX_MODELS = 256;

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
		float roughness;
		float metallic;
		float ao;
		float r;
		float g;
		float b;
	};

	struct DummyTextures
	{
		// dummy textures
		VulkanTexture *diffuse;
		VulkanTexture *normal;
		VulkanTexture *roughness;
		VulkanTexture *metallic;
		VulkanTexture *ao;
	};

	struct Material
	{
		Material() : matTypes(0) {}

		std::string name;
		MaterialProperties properties;
		uint32_t matTypes;

		struct Textures
		{
			Textures() :
				diffuse(nullptr),
				specular(nullptr),
				normal(nullptr),
				roughness(nullptr),
				metallic(nullptr),
				ao(nullptr)
			{}

			VulkanTexture *diffuse;
			VulkanTexture *specular;		
			VulkanTexture *normal;
			VulkanTexture *roughness;		// PBR
			VulkanTexture *metallic;		// PBR
			VulkanTexture *ao;				// PBR
		}  texture;

		VkDescriptors *descriptor;			// descriptor layout the same for all materials, only imageview/samplers differ
	};

	struct SsboLayout
	{
		glm::mat4 projection;
		glm::mat4 viewMatrix;
		std::array<glm::mat4, MAX_MODELS> modelMatrix;
		glm::mat4 boneTransform[MAX_BONES];
	};

	VulkanModel(VulkanEngine *engine, MemoryAllocator *memory);
	virtual ~VulkanModel();

	void Init();
	void Update(int acc_time) override;

	void PrepareMaterialDescriptorSets(Material *material);
	void PreparePipeline();
	void GenerateModelCmdBuffer(VkCommandBuffer cmdBuffer, bool drawShadow = false);

	// mesh-material vertex preperation functions
	void ImportDummyTextures();
	void ProcessMeshes();
	void ProcessMaterials();
	void LoadMaterialTexture(MeshComponentManager::OMFMaterial &material, MaterialType type, VulkanTexture *texture);
	uint8_t FindMaterialIndex(std::string matName);

	// helper functions
	ModelInfo& RequestModelInfo(const uint32_t index) { return m_modelInfo[index]; }
	VkBuffer& GetVertexBuffer() { return p_vkMemory->blockBuffer(m_vertexBuffer.block_id); }
	VkBuffer& GetIndexBuffer() { return p_vkMemory->blockBuffer(m_indexBuffer.block_id); }
	uint32_t GetVertexOffset() const { return m_vertexBuffer.offset; }
	uint32_t GetIndexOffset() const { return m_indexBuffer.offset; }

	friend class VulkanEngine;

private:

	void Destroy() override;

	VulkanEngine *p_vkEngine;

	// buffer info for the "mega" buffer which holds all the models and are referenced via offsets
	MemoryAllocator::MemorySegment m_vertexBuffer;
	MemoryAllocator::MemorySegment m_indexBuffer;
	MemoryAllocator::MemorySegment m_ssboBuffer;

	// pipeline data
	VulkanUtility::PipeLlineInfo m_pipelineInfo;
	std::array<VkPipelineShaderStageCreateInfo, 2> m_shader;

	// data for all models
	std::vector<ModelInfo> m_modelInfo;

	// material data used by pipline
	std::vector<Material> m_materials;

	DummyTextures m_dummyTexture;
};

