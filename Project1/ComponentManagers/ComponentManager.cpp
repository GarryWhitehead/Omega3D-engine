#include "ComponentManager.h"
#include "Engine/World.h"
#include "Engine/engine.h"

ComponentManager::ComponentManager()
{
}


ComponentManager::~ComponentManager()
{
}

// Serialisation functions
void ComponentManager::Serialise(Archiver *arch, ComponentManager& comp, const Archiver::var_info& info)
{
	comp.pack_unpack(arch, info);

}

void ComponentManager::pack_unpack(Archiver *arch, const Archiver::var_info& info)
{}