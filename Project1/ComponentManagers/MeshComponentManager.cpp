#include "MeshComponentManager.h"
#include "Engine/ObjectManager.h"
#include "Engine/engine.h"

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

	// register with graphics system as the updated mesh data will be required for rendering
	RegisterWithSystem(SystemId::GRAPHICS_SYSTEM_ID);
}

void MeshComponentManager::Update()
{

}

void MeshComponentManager::DownloadMeshIndicesData(std::vector<uint32_t>& indicesData) 
{
	indicesData.resize(m_data.meshIndex.size());
	indicesData = m_data.meshIndex; 
}

void MeshComponentManager::Destroy()
{

}

// Serialisation functions
void MeshComponentManager::Serialise(Archiver* arch, MeshComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for physics component manager.......";

	arch->Serialise<uint32_t>(manager.m_data.meshIndex, Archiver::var_info(info.name + ".m_data.meshIndex"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));		// a custom specific serialiser is used for the vector objects

	*g_filelog << " Successfully imported!\n";

	// update object indicies with newly imported data. If the data is being serialised, then this won't have any effect as the object will already be present in the map
	uint32_t index = m_data.object.size() - 1;
	m_indicies.insert(std::make_pair(m_data.object[index], index));
}