#pragma once
#include "ComponentManagers/ComponentManager.h"
#include "VulkanCore/vulkan_utility.h"
#include "Engine/Object.h"
#include <vector>

class VulkanEngine;

enum class MaterialType
{
	NO_TYPE,
	ALBEDO_TYPE,
	AMBIENT_TYPE,
	SPECULAR_TYPE,
	NORMAL_TYPE,
	BUMP_TYPE,
	ROUGHNESS_TYPE,
	METALLIC_TYPE,
	AO_TYPE
};

class MeshComponentManager : public ArchivableComponentManager<MeshComponentManager>
{

public:

	// data structs used by the OMF format
	struct ComponentData
	{
		std::vector<Object> object;
		std::vector<uint32_t> meshIndex;
		std::vector<std::string> materialName;			// user dfeined material name to override default material. Useful for using the same meshes (e.g. geospheres) with different materials. If empty, default is used. 
	};
	
	struct OMFFaceInfo
	{
		std::string material;
		std::vector<uint32_t> indices;
	};

	struct OMFMesh
	{
		std::string meshName;
		std::string material;

		std::vector<glm::vec3> posData;
		std::vector<glm::vec3> normData;
		std::vector<glm::vec2> uvData;
		std::vector<glm::vec3> colorData;
		std::vector<OMFFaceInfo> faceData;
	};

	struct OMFMaterial
	{
		std::string name;

		struct MatColor
		{
			glm::vec3 ambient;
			glm::vec3 diffuse;
			glm::vec3 specular;
			float roughness;
			float metallic;
			float ao;
		} Color;

		struct TexMap
		{
			std::string albedo;
			std::string specular;
			std::string normal;
			std::string ao;
			std::string metallic;
			std::string roughness;
		} Map;

		float opacity;
		uint32_t illumination;

		std::string GetMaterialType(MaterialType type) const;
		bool hasTexture(MaterialType type) const;
		bool hasTexture() const;
		
	};

	struct OMFModel
	{
		std::vector<OMFMesh> meshData;
		std::vector<OMFMaterial> materials;
	};

	MeshComponentManager();
	~MeshComponentManager();

	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;
	void ImportOMFFile(std::string filename);
	void DownloadMeshIndicesData(std::vector<uint32_t>& indicesData);

	// serialisation functions - for general component data
	void Serialise(Archiver* arch, MeshComponentManager& manager, const Archiver::var_info& info);

	// for meshes / materials
	void Serialise(Archiver* arch, OMFMaterial& material, const Archiver::var_info& info);
	void Serialise(Archiver* arch, OMFFaceInfo& face, const Archiver::var_info& info);
	void Serialise(Archiver* arch, OMFMesh& mesh, const Archiver::var_info& info);
	void Serialise(Archiver* arch, OMFModel& model, const Archiver::var_info& info);
	
	template <typename T>
	void Serialise(Archiver *arch, std::vector<T>& vec, const Archiver::var_info& info);

	friend class VulkanModel;

private:

	ComponentData m_data;

	std::vector<OMFModel> m_models;

	// a index loop-up map for identifying the index of entites within the component data
	std::unordered_map<Object, uint32_t, HashGameObj> m_indicies;
};

template <typename T>
void MeshComponentManager::Serialise(Archiver *arch, std::vector<T>& vec, const Archiver::var_info& info)
{
	uint32_t vecSize = static_cast<uint32_t>(vec.size());
	arch->Serialise(vecSize, Archiver::var_info(info.name + ".size"));
	vec.resize(vecSize);

	for (uint32_t c = 0; c < vecSize; ++c) {

		Serialise(arch, vec[c], Archiver::var_info(info.name + "[" + std::to_string(c) + "]"));
	}
}

