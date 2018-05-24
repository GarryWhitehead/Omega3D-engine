#include "PhysicsComponentManager.h"
#include "Engine/engine.h"
#include "Engine/ObjectManager.h"

PhysicsComponentManager::PhysicsComponentManager() :
	ArchivableComponentManager<PhysicsComponentManager>(*this)
{
}

PhysicsComponentManager::~PhysicsComponentManager()
{
	Destroy();
}

void  PhysicsComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;
}

void PhysicsComponentManager::Update()
{
	for (uint32_t c = 0; c < m_data.object.size(); ++c) {

		m_data.velocity[c] += m_data.acceleration[c] * Engine::DT;
		m_data.position[c] += m_data.velocity[c] * Engine::DT;
	}
}

void PhysicsComponentManager::Destroy()
{

}

void PhysicsComponentManager::DownloadPhysicsData(std::vector<UpdatedPhysics>& updatedData)
{
	updatedData.resize(m_indicies.size());
	for (int c = 0; c < m_indicies.size(); ++c) {

		UpdatedPhysics update;
		update.pos.push_back(m_data.position[c]);
		update.rot.push_back(m_data.rotation[c]);
		update.scale.push_back(m_data.scale[c]);
		updatedData[c] = update;
	}
}

void PhysicsComponentManager::DownloadObjectIndex(std::unordered_map<Object, uint32_t, HashGameObj>& objectIndex)
{
	objectIndex = m_indicies;
}

// Serialisation functions
void PhysicsComponentManager::Serialise(Archiver* arch, PhysicsComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for physics component manager.......";

	arch->Serialise<glm::vec3>(manager.m_data.acceleration, Archiver::var_info(info.name + ".m_data.acceleration"));
	arch->Serialise<glm::vec3>(manager.m_data.velocity, Archiver::var_info(info.name + ".m_data.velocity"));
	arch->Serialise<glm::vec3>(manager.m_data.position, Archiver::var_info(info.name + ".m_data.position"));
	arch->Serialise<glm::vec4>(manager.m_data.rotation, Archiver::var_info(info.name + ".m_data.rotation"));
	arch->Serialise<glm::vec3>(manager.m_data.scale, Archiver::var_info(info.name + ".m_data.scale"));
	arch->Serialise<float>(manager.m_data.mass, Archiver::var_info(info.name + ".m_data.mass"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));		// a custom specific serialiser is used for the vector objects

	*g_filelog << " Successfully imported!\n";

	// update object indicies with newly imported data. If the data is being serialised, then this won't have any effect as the object will already be present in the map
	for (int c = 0; c < m_data.object.size(); ++c) {
		m_indicies.insert(std::make_pair(m_data.object[c], c));
	}
}