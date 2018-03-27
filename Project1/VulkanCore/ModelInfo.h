#pragma once
#include <vector>
#include "VulkanCore/vulkan_utility.h"
#include "Objector/objector.h"
#include "Objector/MaterialParser.h"
#include "glm.hpp"

struct ModelInfo
{
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

		// each material has its own descriptort set
		VkDescriptorSet descrSet;
	};

	struct MeshData
	{
		uint32_t indexBase;
		uint32_t indexCount;
		uint32_t vertexBase;

		Material *material;

		std::vector<VkDescriptorSet> descrSets;
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
	};

	// generic vertex layout used (hopefully) by all model-based vertex shaders
	struct ModelVertex
	{
		VkVertexInputBindingDescription GetInputBindingDescription();
		std::array<VkVertexInputAttributeDescription, 4> GetAttrBindingDescription();

		glm::vec3 pos;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec3 colour;
	};

	// constructor/de-constructor
	ModelInfo();
	ModelInfo(const ModelInfo& model);
	~ModelInfo();

	// function templates
	void LoadModel(std::string filename, VkDevice device, VkCommandPool cmdPool);
	TextureInfo LoadMaterialTexture(objMaterial &material, objTextureType type, VkCommandPool cmdPool);
	void ProcessMeshes(Objector::ModelInfo *model);
	void ProcessMaterials(Objector::ModelInfo *model, VkCommandPool cmdPool);

	// data
	Objector objector;

	std::vector<ModelVertex> vertices;
	std::vector<uint32_t> indices;

	std::vector<MeshData> meshData;

	std::vector<Material> materialData;
	BufferData indexBuffer;
	BufferData vertexBuffer;
};

