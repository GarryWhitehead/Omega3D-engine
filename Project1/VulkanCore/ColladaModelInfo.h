#pragma once
#include "SiimpleCollada/SimpleCollada.h"
#include "VulkanCore/vulkan_utility.h"
#include <array>
#include <map>

static const int MAX_VERTEX_BONES = 4;

enum class MaterialTexture
{
	TEX_DIFF = 1 << 0,
	TEX_NORM = 1 << 1,
	TEX_NONE = 1 << 2
};

struct ColladaModelInfo
{
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

	struct FaceInfo
	{
		uint32_t materialIndex;
		uint32_t indexBase;
		uint32_t indexCount;
	};

	struct MeshInfo
	{
		uint32_t vertexBase;
		uint32_t vertexCount;

		std::vector<FaceInfo> faceInfo;
	};

	struct MaterialProperties
	{
		glm::vec4 ambient;
		glm::vec4 diffuse;
		glm::vec4 specular;
		glm::vec4 color;
		float shininess;
		float transparency;
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
			TextureInfo specular;
			TextureInfo normal;
			TextureInfo nomap;			// dummy texture
		}  texture;

		VkDescriptorSet descrSet;
		VkPipeline pipeline;
		VkPipelineLayout layout;
	};

	struct VertexBoneInfo
	{
		std::array<float, MAX_VERTEX_BONES> weights;
		std::array<uint32_t, MAX_VERTEX_BONES> boneId;		
	};

	struct SkeletonInfo
	{
		std::string name;
		glm::mat4 invBind;
		glm::mat4 localTransform;
		glm::mat4 finalTransform;
	};

	ColladaModelInfo();
	~ColladaModelInfo();

	void ImportFile(std::string filename, VulkanEngine *vkEngine, VkCommandPool cmdPool);
	void ProcessMeshes();
	void ProcessMaterials(VulkanEngine *vkEngine, VkCommandPool cmdPool);
	void AddVertexBoneData(uint32_t index, uint32_t id, float weight);
	uint8_t FindMaterialIndex(std::string mat);
	TextureInfo LoadMaterialTexture(ColladaMaterials::MaterialInfo &material, MatType type, VulkanEngine *vkEngine, VkCommandPool cmdPool);
	void UpdateModelAnimation();

	// helper functions
	uint32_t numMeshes() const { return m_meshData.size(); }
	uint32_t numMaterials() const { return m_materials.size(); }

	// data storage
	SimpleCollada *p_colladaImporter;

	std::vector<MeshInfo> m_meshData;
	std::vector<Material> m_materials;
	std::vector<SkeletonInfo> m_boneData;
	std::vector<VertexBoneInfo> m_vertexBoneData;
	std::vector<glm::mat4> m_boneTransforms;
	glm::mat4 m_globalInvTransform;
	SimpleCollada::SceneData *p_scene;

	BufferData vertexBuffer;
	BufferData indexBuffer;
	std::vector<ModelVertex> vertices;
	std::vector<uint32_t> indices;

	std::map<std::string, uint32_t> m_boneMap;
};

