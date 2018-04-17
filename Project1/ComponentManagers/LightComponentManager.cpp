#include "LightComponentManager.h"


LightComponentManager::LightComponentManager(ComponentManagerId id) :
	ArchivableComponentManager<LightComponentManager>(*this, id)
{
}


LightComponentManager::~LightComponentManager()
{
}
