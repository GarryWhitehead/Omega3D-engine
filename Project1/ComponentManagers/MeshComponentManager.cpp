#include "MeshComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/AnimationComponentManager.h"
#include "Engine/ObjectManager.h"
#include "Engine/engine.h"
#include "VulkanCore/VulkanEngine.h"

MeshComponentManager::MeshComponentManager(ComponentManagerId id) :
	ArchivableComponentManager<MeshComponentManager>(*this, id)
{
}


MeshComponentManager::~MeshComponentManager()
{
}

void MeshComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;
}

void MeshComponentManager::Update()
{

}
 
void MeshComponentManager::ImportOMFFile(std::string filename)
{
	// de-serialise data from file
	Archiver *p_archiver = new Archiver();
	p_archiver->OpenFile(filename, SaveMode::SAVE_TEXT, FileMode::FILE_IN);

	Serialise(p_archiver, m_models, Archiver::var_info(""));

	*g_filelog << "Successfully imported OMF file: " << filename << "\n";
}


void MeshComponentManager::DownloadMeshIndicesData(std::vector<uint32_t>& indicesData)
{
	indicesData.resize(m_data.meshIndex.size());

	for (int c = 0; c < m_data.object.size(); ++c) {

		indicesData.push_back(m_data.meshIndex[c]);
	}
}

void MeshComponentManager::Destroy()
{

}

// Serialisation functions
void MeshComponentManager::Serialise(Archiver* arch, MeshComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for mesh component manager.......";

	arch->Serialise<uint32_t>(manager.m_data.meshIndex, Archiver::var_info(info.name + ".m_data.meshIndex"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));		// a custom specific serialiser is used for the vector objects

	*g_filelog << " Successfully imported!\n";

	// update object indicies with newly imported data. If the data is being serialised, then this won't have any effect as the object will already be present in the map
	uint32_t index = m_data.object.size() - 1;
	m_indicies.insert(std::make_pair(m_data.object[index], index));
}

// material serialisation functions
void MeshComponentManager::Serialise(Archiver* arch, OMFMaterial& material, const Archiver::var_info& info)
{
	// materials colour
	arch->Serialise(material.Color.ambient, info.name + "roughness");
	arch->Serialise(material.Color.diffuse, info.name + "roughness");
	arch->Serialise(material.Color.specular, info.name + "roughness");
	arch->Serialise(material.Color.roughness, info.name + "roughness");
	arch->Serialise(material.Color.metallic, info.name + "metallic");
	
	// materials texture maps
	arch->Serialise(material.Map.albedo, info.name + "albedo");
	arch->Serialise(material.Map.ao, info.name + "ao");
	arch->Serialise(material.Map.metallic, info.name + "metallic");
	arch->Serialise(material.Map.normal, info.name + "normal");
	arch->Serialise(material.Map.roughness, info.name + "roughness");
	arch->Serialise(material.Map.specular, info.name + "specular");

	arch->Serialise(material.illumination, info.name + "illumination");
	arch->Serialise(material.opacity, info.name + "opacity");
	arch->Serialise(material.name, info.name + "name");
}

// mesh serialisation fucntions
void MeshComponentManager::Serialise(Archiver* arch, OMFFaceInfo& face, const Archiver::var_info& info)
{
	arch->Serialise<uint32_t>(face.indices, info.name + "indices");
}

void MeshComponentManager::Serialise(Archiver* arch, OMFMesh& mesh, const Archiver::var_info& info)
{
	arch->Serialise(mesh.meshName, info.name + "name");
	arch->Serialise(mesh.material, info.name + "material");
	arch->Serialise<glm::vec3>(mesh.posData, info.name + "position");
	arch->Serialise<glm::vec2>(mesh.uvData, info.name + "uv");
	arch->Serialise<glm::vec3>(mesh.normData, info.name + "normal");
	arch->Serialise<glm::vec3>(mesh.colorData, info.name + "color");
	Serialise<OMFFaceInfo>(arch, mesh.faceData, info.name + "face");
}

// model serialisation fucntions
void MeshComponentManager::Serialise(Archiver* arch, OMFModel& model, const Archiver::var_info& info)
{
	Serialise<OMFMesh>(arch, model.meshData, info.name + "meshData");
	Serialise<OMFMaterial>(arch, model.materials, info.name + "materials");
}

// Material functions
std::string MeshComponentManager::OMFMaterial::GetMaterialType(MaterialType type) const
{
	std::string filename;
	switch (type) {
	case MaterialType::ALBEDO_TYPE:
		filename = Map.albedo;
		break;
	case MaterialType::NORMAL_TYPE:
		filename = Map.normal;
		break;
	case MaterialType::SPECULAR_TYPE:
		filename = Map.specular;
		break;
	case MaterialType::ROUGHNESS_TYPE:
		filename = Map.roughness;
		break;
	case MaterialType::METALLIC_TYPE:
		filename = Map.metallic;
		break;
	}
	return filename;
}

bool MeshComponentManager::OMFMaterial::hasTexture(MaterialType type) const
{
	switch (type) {
	case MaterialType::ALBEDO_TYPE:
		return !Map.albedo.empty();
		break;
	case MaterialType::NORMAL_TYPE:
		return !Map.normal.empty();
		break;
	case MaterialType::SPECULAR_TYPE:
		return !Map.specular.empty();
		break;
	case MaterialType::ROUGHNESS_TYPE:
		return !Map.roughness.empty();
		break;
	case MaterialType::METALLIC_TYPE:
		return !Map.metallic.empty();
		break;
	}
}

bool MeshComponentManager::OMFMaterial::hasTexture() const
{
	// all materials must have a diffuse texture
	if (Map.albedo.empty()) {
		return false;
	}
	return true;
}

