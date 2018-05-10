#include "ComponentManager.h"
#include "Engine/World.h"
#include "Engine/engine.h"

ComponentManager::ComponentManager(ComponentManagerId id) :
	m_id(id)
{
}


ComponentManager::~ComponentManager()
{
}

void ComponentManager::RegisterWithManager(ComponentManagerId dstId)
{
	p_world->LinkComponentManager(m_id, dstId);
}

void ComponentManager::RegisterManager(ComponentManager *manager)
{
	assert(manager != nullptr);
	m_registeredManagers.push_back(manager);
}

bool ComponentManager::HasRegisteredManager(ComponentManagerId id)
{
	for (auto man : m_registeredManagers) {

		if (man->m_id == id) {
			return true;
		}
	}
	return false;
}

// Serialisation functions
void ComponentManager::Serialise(Archiver *arch, ComponentManager& comp, const Archiver::var_info& info)
{
	comp.pack_unpack(arch, info);

}

void ComponentManager::pack_unpack(Archiver *arch, const Archiver::var_info& info)
{}