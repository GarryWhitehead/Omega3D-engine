#include "Systems/system.h"
#include <assert.h>

System::System()
{
}

System::~System() 
{
}

void System::RegisterManager(ComponentManager *manager)
{
	assert(manager != nullptr);
	m_registeredManagers.push_back(manager);
}


